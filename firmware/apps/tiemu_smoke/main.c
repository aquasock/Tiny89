/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Tiny89 on the ae350_soc fabric: run the emulated TI-89 Titanium
 * continuously, presenting the calculator's LCD at the platform's vertical
 * blank and feeding its keypad from a Wii Classic Controller.
 *
 * Two video units answer at the same registers.  The inherited TinyNES unit
 * (id "TNV1") shows a 1-bit 256x240 frame buffer; the new Tiny89 grayscale
 * unit (id "T89V") reads a 160x100 four-level image out of the top 128 KiB of
 * SDRAM instead (firmware/common/t89_lcd.c).  The unit is probed once at
 * start-up and the same ELF keeps working on either.
 */
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "gscales.h"
#include "hw.h"
#include "images.h"
#include "kbd.h"
#include "libuae.h"
#include "m68k.h"
#include "load.h"
#include "tinynes.h"
#include "t89_bios.h"
#include "t89_lcd.h"
#include "t89_key.h"
#include "t89_save.h"
#include "tiny89_platform.h"
#include "ti68k_def.h"

#define FLASH_BYTES T89_BIOS_BYTES

/* "TNR1" as the little-endian word the loader puts in front of a file it is
 * asked for, then the file's length: TinyNES's framing, which TinyTang's
 * ae350run uses to serve a file from the card. */
#define T89_TNR1_MAGIC 0x31524e54u

/*
 * One 60 Hz video frame of emulated time.  The TI-89 Titanium's CPU is 12 MHz
 * (TiEmu, core/ti_hw/hw.c, OSC1), so one frame is 12e6 / 60 = 200000 M68K
 * cycles.  hw_m68k_run() stops at whichever limit comes first, and here the
 * cycle limit is the one that binds: the instruction limit is only a guard
 * against a run of very cheap opcodes.
 *
 * The interpreter on this AE350 is far slower than real time, so the loop is
 * never paced to the wall clock -- it runs flat out and presentation follows
 * the fabric's vertical blanks.
 */
#define FRAME_INSTRUCTIONS 2000000
#define FRAME_CYCLES       200000u

#define READY_FRAMES        300u /* presented frames that satisfy READY */
#define READY_STABLE_FRAMES 30u  /* or this many unchanged LCD frames */

/*
 * One status line per this many presented frames.  3600 is about a minute at
 * the fabric's 60 Hz, and is the value TinyNES uses for NES_REPORT in its
 * board build, so the calculator sits on the console as quietly as a game
 * does.  A DIAG=1 build asks for the per-second stream instead, which is what
 * TinyNES's own DIAG=1 build does and what every earlier Tiny89 cycle read on
 * the console to check the emulator and the keyboard.
 */
#ifdef T89_DIAG
#define HEARTBEAT_FRAMES    60u
#else
#define HEARTBEAT_FRAMES    3600u
#endif
#define MAX_KEY_EVENTS      32

static uint32_t lcd_hash(void)
{
    const uint8_t *data = &tihw.ram[tihw.lcd_adr & (tihw.ram_size - 1)];
    uint32_t hash = UINT32_C(2166136261);
    size_t index;
    for (index = 0; index < 30u * 128u; index++) {
        hash ^= data[index];
        hash *= UINT32_C(16777619);
    }
    return hash;
}

/*
 * The calculator's 160x100 monochrome LCD (30 bytes per row, MSB first),
 * centred at x 48 y 70 in the 256x240 frame buffer.  Pixel 0 is the LCD's
 * background, pixel 1 its ink; the palette is set once at start-up.  The
 * frame buffer is written through the D-cache, so callers must write the
 * D-cache back before asking the fabric to show the buffer.
 */
static void render_lcd(uint8_t *frame)
{
    const uint8_t *lcd = &tihw.ram[tihw.lcd_adr & (tihw.ram_size - 1)];
    unsigned x, y;

    memset(frame, 0, 0x10000u);
    for (y = 0; y < 100; y++) {
        for (x = 0; x < 160; x++) {
            unsigned set = lcd[y * 30u + x / 8u] & (0x80u >> (x & 7));
            frame[(70u + y) * T89_VIDEO_WIDTH + 48u + x] = set ? 1 : 0;
        }
    }
}

/*
 * Put the calculator's picture into the buffer the fabric is not showing.
 *
 * The T89V unit reads its own 160x100 four-level plane image at SDRAM
 * 0x41fe0000 + 0x10000*b and shades it from PALETTE(0..3); the inherited
 * TinyNES unit keeps the 1-bit 256x240 frame buffer it has always had.  Both
 * write through the D-cache, so the caller writes it back before presenting.
 */
static void render_frame(int gray, unsigned buffer)
{
    uint8_t *frame = T89_VIDEO_FB(buffer);

    if (gray) {
        t89_lcd_render(frame);
    } else {
        render_lcd(frame);
    }
}

/*
 * Sample the keyboard the desk link holds and hand the difference to TiEmu's
 * keypad.
 *
 * TinyTang pairs the Bluetooth keyboard and forwards its HID boot report to
 * the core as an input frame (type 0x17), which this platform's fabric parses
 * in desk_link.sv into the two registers the app reads here: the high one
 * carries the newest report's last three usages and a sequence byte that
 * advances whenever a frame arrives, and the low one carries the modifier
 * byte and the first three usages.  So the sequence byte says whether to read
 * at all, and the report is state rather than events: a key held down sends
 * one report and then nothing, which is why the module diffs a report against
 * the one it last applied rather than counting presses.
 *
 * Two uncached register reads and one diff cost nothing beside a frame of
 * interpreted M68K.
 *
 * Returns the number of key events applied, or a negative TiEmu/module error.
 */
/* How many calculator keys the keyboard has pressed just now, for the
 * heartbeat: the module's own applied set is the state the emulator was told
 * about. */
static int keys_down(void)
{
    int key, n = 0;

    for (key = 0; key < NB_MAX_KEYS; key++) {
        if (ti68k_kbd_is_key_pressed(key)) n++;
    }
    return n;
}

static int poll_keyboard(uint8_t *last_sequence, t89_key_event_t *events,
                         int *command)
{
    uint32_t hi = T89_KBD_HI;
    uint8_t sequence = (uint8_t)(hi >> 24);
    t89_kbd_t kbd;
    int count, i;

    if (command) *command = T89_KBD_CMD_NONE;
    if (sequence == *last_sequence) return 0;
    *last_sequence = sequence;

    t89_kbd_decode(T89_KBD_LO, hi, &kbd, 0);
    /* A report is state, so a command arrives once, with the report that
     * carries it; holding the chord does not repeat it. */
    if (command) *command = t89_kbd_command(&kbd);

    count = t89_kbd_update(&kbd, events, MAX_KEY_EVENTS);
    if (count < 0) return count;

    for (i = 0; i < count; i++) {
        if (events[i].key < 0 || events[i].key >= NB_MAX_KEYS) return -1;
        ti68k_kbd_set_key(events[i].key, events[i].active);
    }
    return count;
}

/*
 * The board files a save a line at a time, and a line sent before it has
 * finished with the last one is a line that goes missing -- which is why
 * TinyNES's save pauses between lines too.  Waiting for the link to be idle is
 * not enough on its own: that says this end has sent everything, not that the
 * board has filed it, so the pause is what the board is given.  Four
 * milliseconds a line is TinyNES's own figure, measured on this board; the
 * calculator's RAM is 8192 lines, so a save takes about 35 seconds, which is
 * why it is a key the user presses rather than something automatic.
 */
static void save_pace(void)
{
    const uint64_t until = t89_cycles64() + T89_CORE_HZ / 250u; /* 4 ms */

    /* The line has to be out before the pause means anything: a line still in
     * the C library's buffer would pace nothing at all. */
    fflush(stdout);
    t89_wait_tx_idle();
    while (t89_cycles64() < until) {
    }
}

/*
 * Hand the calculator's RAM to the board, which writes it beside the other
 * saves on the card.  This is TinyNES's NVRAM protocol: a line naming the
 * length and the CRC-32, the bytes as hex lines, and the CRC again at the end.
 * ae350run decodes the hex into a binary .sav and throws the file away if the
 * length or the CRC disagree, so a save lost in the link is discarded instead
 * of being loaded as a broken calculator next time.
 *
 * When this runs the emulator is not: the picture freezes for the length of
 * the save, which is the price of getting the state onto the card.
 */
static void save_ram(void)
{
    char line[T89_SAVE_LINE_CHARS + 1];
    const size_t bytes = (size_t)tihw.ram_size;
    uint32_t crc;
    size_t offset;
    int lead;

    /* The CPU wrote this RAM through the D-cache, so it is written back
     * before it is read (tinynes.h; TinyNES does the same before a save). */
    t89_dcache_wbinval();
    crc = t89_save_crc32(0, tihw.ram, bytes);

    /* The header is not the first thing sent: a burst's opening is where the
     * link loses a chunk, and on 2026-10-14 TinyNES's own header arrived as
     * "TinyNES N", which armed no save at all.  Warm the link first. */
    for (lead = 0; lead < 3; lead++) {
        printf("TinyNES NVRAM BEGIN\n");
        save_pace();
    }

    printf("TinyNES NVRAM SAVE %lu %08" PRIx32 "%s\n", (unsigned long)bytes,
           crc, t89_save_is_blank(tihw.ram, bytes) ? " BLANK" : "");
    save_pace();

    for (offset = 0; offset < bytes; offset += T89_SAVE_LINE_BYTES) {
        t89_save_hex(line, tihw.ram + offset, T89_SAVE_LINE_BYTES);
        printf("%s\n", line);
        save_pace();
    }

    printf("TinyNES NVRAM DONE %08" PRIx32 "\n", crc);
    t89_wait_tx_idle();
    save_pace();
    printf("Tiny89 RAM SAVED %lu %08" PRIx32 "\n", (unsigned long)bytes, crc);
}

int main(void)
{
    uint8_t *flash;
    uint8_t *upgrade;
    t89_key_event_t events[MAX_KEY_EVENTS];
    const char *video;
    uint32_t video_id, frames, last_frames, last_lcd = 0;
    uint32_t presented = 0, stable = 0;
    uint8_t last_sequence = 0;
    unsigned buffer = 0;
    int gray, ready_printed = 0, command;

    printf("Tiny89 AE350 TiEmu smoke start\n");

    /* The BIOS comes off the card the way a cartridge does.  The program asks
     * the loader for a file at an address it chooses and receives "TNR1", the
     * length and the file -- TinyNES's protocol, whose "TinyNES WANT <address>
     * <bytes>" line TinyTang's ae350run answers from the card.  What the user
     * provides is the upgrade TI ships, so it is converted here into the 4 MiB
     * image the emulator boots (firmware/common/t89_bios.c). */
    upgrade = malloc(T89_BIOS_MAX + 8);
    flash = malloc(T89_BIOS_BYTES);
    if (upgrade == NULL || flash == NULL) {
        printf("BAD Tiny89 BIOS allocation\n");
        return 1;
    }

    printf("TinyNES WANT %08lx %08lx\n", (unsigned long)(uintptr_t)upgrade,
           (unsigned long)(T89_BIOS_MAX + 8));
    fflush(stdout);
    tn_load_reset();
    {
        uint32_t got = 0;
        int second;

        for (second = 0; second < 30 && got == 0; second++) {
            got = tn_load_poll(TN_CORE_HZ);
        }
        if (got != (uint32_t)(uintptr_t)upgrade) {
            printf("BAD Tiny89 BIOS not loaded\n");
            return 1;
        }
    }
    {
        uint32_t magic, length;
        char why[64];

        memcpy(&magic, upgrade, 4);
        memcpy(&length, upgrade + 4, 4);
        if (magic != T89_TNR1_MAGIC || length == 0 || length > T89_BIOS_MAX) {
            printf("BAD Tiny89 BIOS framing\n");
            return 1;
        }
        if (t89_bios_from_89u(upgrade + 8, length, flash, why, sizeof why) != 0) {
            printf("BAD Tiny89 BIOS %s\n", why);
            return 1;
        }
        printf("Tiny89 BIOS %lu bytes of upgrade\n", (unsigned long)length);
    }
    free(upgrade);

    memset(&img_infos, 0, sizeof(img_infos));
    img_infos.data = flash;
    img_infos.calc_type = TI89t;
    memcpy(img_infos.version, "3.10", 5);
    img_infos.flash = FLASH_ROM;
    img_infos.has_boot = 0;
    img_infos.size = FLASH_BYTES;
    img_infos.hw_type = HW3;
    img_infos.rom_base = 0x80;
    img_loaded = 1;

    params.restricted = 0;
    params.cpu_rate = -1;
    params.hw_rate = -1;
    params.lcd_rate = -1;
    params.hw_protect = 1;
    params.recv_file = 0;

    if (hw_init() || hw_reset()) {
        printf("BAD Tiny89 hardware initialization\n");
        return 1;
    }
    printf("Tiny89 BOOT SSP=%08" PRIx32 " PC=%08" PRIx32 "\n",
        tihw.initial_ssp, tihw.initial_pc);

    /*
     * The calculator's RAM, if a previous session left one on the card.  The
     * board serves a save headerless straight into the buffer named here and
     * the loader's own block checksums judge it, so the file lands in tihw.ram
     * the way a battery-backed calculator's RAM is found at power-up.  A first
     * boot has no file, ae350run says so, and the zeroed RAM TiEmu allocated
     * is what the OS then cold-starts on.
     *
     * The request goes out now, while the UART is still in loader mode: once
     * TinyNES LINK is printed the line carries text frames and cannot ask for
     * a file any more.
     */
    printf("TinyNES NVRAM WANT %08" PRIx32 " %08" PRIx32 "\n",
           (uint32_t)(uintptr_t)tihw.ram, (uint32_t)tihw.ram_size);
    fflush(stdout);
    tn_load_reset();
    {
        uint32_t got = 0;
        int tenth;

        for (tenth = 0; tenth < 10 && got == 0; tenth++) {
            got = tn_load_poll(TN_CORE_HZ / 10);
        }
        if (got == (uint32_t)(uintptr_t)tihw.ram) {
            printf("Tiny89 RAM GOT %lu %08" PRIx32 "\n",
                   (unsigned long)tihw.ram_size,
                   t89_save_crc32(0, tihw.ram, (size_t)tihw.ram_size));
        } else {
            printf("Tiny89 RAM none yet\n");
        }
    }

    /*
     * Seed TiEmu's gray-plane model exactly as the TiEmu GTK frontend did
     * after a reset, gui/calc/calc.c:587-590 (the same three lines reappear in
     * gscales.c:418-420 for a "refresh from GTK"): plane 0 is the LCD at
     * tihw.lcd_adr, its buffer is the calculator's RAM at that address, and one
     * plane is in use.  Without it lcd_planebufs[0] stays NULL until
     * gscales.c's process_address() has seen UPDATE_PLANES (16) plane scans,
     * which the T89V render path would read as an all-paper plane.  The model
     * itself is driven from the run loop by core/ti_hw/m68k.c:263,
     * lcd_hook_hw2(0), for HW2/HW3.
     */
    lcd_planes[0] = tihw.lcd_adr;
    lcd_planebufs[0] = &tihw.ram[tihw.lcd_adr];
    ngc = 1;
    lcd_changed = 1;

    /*
     * Which video unit answers: the new Tiny89 grayscale unit reads the
     * 160x100 four-level image from the top of SDRAM, the inherited TinyNES
     * unit the 1-bit 256x240 frame buffer.  Probed once; the render path
     * follows it.
     */
    video_id = T89_VIDEO_ID;
    gray = (video_id == T89_VIDEO_ID_T89V);
    video = gray ? "T89V" : "TNV1";
    if (gray) {
        printf("Tiny89 VIDEO T89V\n");
    } else {
        printf("Tiny89 VIDEO TNV1 ID=%08" PRIx32 "\n", video_id);
    }

    /* Start showing the calculator's LCD: palette first, then the content of
     * buffer 0, written back before the fabric is told to show it. */
    if (gray) {
        /* The T89V unit's four grays, paper to ink: a project choice, made by
         * interpolating the palette Tiny89 used for its 1-bit display (paper
         * 0x9fb58f, ink 0x182018), so level 0
         * and level 3 are still that paper and ink and levels 1 and 2 sit
         * evenly between them (45/50/40 per channel step, the same linear
         * spacing TiEmu's compute_grayscale() used). */
        T89_VIDEO_PALETTE(0) = 0x9fb58fu;
        T89_VIDEO_PALETTE(1) = 0x728367u;
        T89_VIDEO_PALETTE(2) = 0x455240u;
        T89_VIDEO_PALETTE(3) = 0x182018u;
    } else {
        T89_VIDEO_PALETTE(0) = 0x9fb58fu;
        T89_VIDEO_PALETTE(1) = 0x182018u;
    }
    render_frame(gray, buffer);
    t89_dcache_wbinval();
    T89_VIDEO_CTRL = T89_VIDEO_ENABLE;

    /*
     * Probe the Wii pad unit once.  The image on the Tang is a copy of a
     * TinyNES SoC build and it is not verified that it decodes 0xe8040000, so
     * a missing pad unit must never fail the boot: the session simply runs
     * with no input.
     */
    t89_kbd_reset();

    /* The keyboard arrives as TinyTang's frames and the fabric's desk link
     * parses them, so the UART has to be in its link mode.  A write to CTRL
     * completes once everything written before it has gone out, so the line
     * below reaches the host as text first; the host loader watches for
     * exactly that line to start reading frames instead of text (TinyNES's
     * tools/load_ae350.py, which is also Tiny89's).  Nothing is printed after
     * this point but frames. */
    printf("TinyNES LINK\n");
    T89_UART_CTRL = T89_UART_LINK_MODE;
    printf("Tiny89 KBD seq=%u\n", (unsigned)(T89_KBD_HI >> 24));

    last_frames = T89_VIDEO_FRAMES;

    for (;;) {
        uint32_t hash;
        int result, count;

        /* (a) One frame of emulated time.  TiEmu refreshes its timers,
         * hardware and keyboard inside hw_m68k_run(), so nothing else is
         * stepped here. */
        result = hw_m68k_run(FRAME_INSTRUCTIONS, FRAME_CYCLES);
        if (result) {
            printf("BAD Tiny89 CPU result=%d\n", result);
            return 1;
        }

        /* (b) Take keyboard input. */
        count = poll_keyboard(&last_sequence, events, &command);
        if (count < 0) {
            printf("BAD Tiny89 keyboard result=%d\n", count);
            return 1;
        }

        /* (b2) Control with s: the program's own key, not the calculator's
         * (t89_key.c maps nothing from a report that carries Control).  The
         * emulator stops for the length of the save. */
        if (command == T89_KBD_CMD_SAVE) {
            save_ram();
        }

        /* (c) Present at the fabric's vertical blank: refresh the buffer that
         * is not being shown, write the D-cache back, then hand it over by
         * flipping CTRL's buffer bit. */
        frames = T89_VIDEO_FRAMES;
        if (frames != last_frames) {
            last_frames = frames;
            buffer ^= 1u;
            render_frame(gray, buffer);
            t89_dcache_wbinval();
            T89_VIDEO_CTRL = T89_VIDEO_ENABLE |
                (buffer ? T89_VIDEO_BUFFER : 0u);
            presented++;

            hash = lcd_hash();
            if (hash == last_lcd) {
                stable++;
            } else {
                last_lcd = hash;
                stable = 0;
            }

            /* (d) The deployment tool waits for this string, so it is printed
             * exactly once, at whichever of the two conditions comes first. */
            if (!ready_printed &&
                (presented >= READY_FRAMES || stable >= READY_STABLE_FRAMES)) {
                ready_printed = 1;
                printf("Tiny89 READY PC=%08" PRIx32 " LCD=%08" PRIx32 "\n",
                    (uint32_t)m68k_getpc(), hash);
            }

            /* (e) The heartbeat, at HEARTBEAT_FRAMES: one line a minute by
             * default, one a second in a DIAG=1 build. */
            if (presented % HEARTBEAT_FRAMES == 0u) {
                printf("Tiny89 FRAME n=%u PC=%08" PRIx32 " CYC=%u"
                       " LCD=%08" PRIx32 " VBL=%u KBDS=%u KEYS=%d"
                       " VID=%s",
                    (unsigned)presented, (uint32_t)m68k_getpc(),
                    (unsigned)hw_m68k_get_cycle_count(0), hash,
                    (unsigned)frames, (unsigned)last_sequence,
                    keys_down(), video);
                if (gray) printf(" LCDMODE=%s", t89_lcd_mode());
                printf("\n");
            }
        }
    }
}
