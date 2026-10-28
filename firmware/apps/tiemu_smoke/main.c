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
#include "t89_rtc.h"
#include "t89_file.h"
#include "t89_link.h"
#include "t89_proto.h"
#include "t89_save.h"
#include "t89_state.h"
#include "tiny89_platform.h"
#include "ti68k_def.h"

#define FLASH_BYTES T89_BIOS_BYTES

/* How much room to ask the loader for when a state is coming back.  A TI-89
 * Titanium state is 256 KiB of RAM plus 544 bytes of ports and registers, so
 * 1 MiB leaves room for the format to grow. */
#define T89_STATE_MAX (1024u * 1024u)

/* A file to hand the calculator over its link, when the board was given one.
 * The largest app in the user's set is 582 KB, so a megabyte holds any. */
#define T89_SEND_MAX (1024u * 1024u)

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
    const uint64_t until =
        t89_cycles64() + (uint64_t)T89_SAVE_PACE_MS * (T89_CORE_HZ / 1000u);

    /* The line has to be out before the pause means anything: bytes still in
     * the C library's buffer would pace nothing at all. */
    fflush(stdout);
    t89_wait_tx_idle();
    while (t89_cycles64() < until) {
    }
}

/* The pause that covers one of the board's card writes.  The board fills a
 * 512-byte buffer and files it, holding the CPU with interrupts masked; a byte
 * that arrives during that window is lost, and the save's length still comes
 * out exact because the board counts its payload down by the length it was
 * given.  Sent at every chunk boundary, this pause is what makes those bytes
 * arrive while the card is busy doing nothing else. */
static void save_chunk_pace(void)
{
    const uint64_t until =
        t89_cycles64() + (uint64_t)T89_SAVE_CHUNK_MS * (T89_CORE_HZ / 1000u);

    fflush(stdout);
    t89_wait_tx_idle();
    while (t89_cycles64() < until) {
    }
}

/* The settle that lets the board finish arming before payload arrives: it
 * opens the temporary file and appends its arm line at exactly this moment,
 * and every one of those card calls masks the CPU. */
static void save_arm_pace(void)
{
    const uint64_t until =
        t89_cycles64() + (uint64_t)T89_SAVE_ARM_MS * (T89_CORE_HZ / 1000u);

    fflush(stdout);
    t89_wait_tx_idle();
    while (t89_cycles64() < until) {
    }
}

/* The flash as this session found it: the save writes it only when the OS has
 * changed it, so an ordinary session costs one RAM save. */
static uint32_t s_flash_boot_crc;

/* The file to hand the calculator, and whether it has gone yet. */
static uint8_t *s_send_blob;
static t89_var_t s_send_var;
static int s_send_done;
static uint32_t s_send_last_try;
static unsigned s_send_tries;
static unsigned s_send_ticks;

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
    const uint64_t started = t89_cycles64();
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
        printf("Tiny89 NVRAM BEGIN\n");
        save_pace();
    }

/* The bytes, by default: measured on 2026-10-23, a 4 MiB flash image went
 * through this shape whole while 256 KiB of hex lost 302 bytes in the same
 * session, so hex is the opt-in (make HEX=1) rather than the default. */
#if !defined(T89_SAVE_HEX)
    /* The payload as the bytes themselves, which is TinyTang's `TinyNES NVRAM
     * DATA` shape and the only one that makes a 4 MiB flash practical: a frame
     * carries 128 bytes, so a pause a frame crosses 256 KiB in 16.6 s against
     * hex's 65.8 s.  Measured on this board, and the measurement is the reason
     * this is not the default -- one of two runs came back with every byte
     * present, nothing dropped, and a CRC that disagreed, because a payload
     * with no line boundaries to re-align on cannot survive a frame going
     * missing; the count still reads complete and the content is shifted.
     * Built with `make BINARY=1`; core-log entry 15 has the numbers. */
    const size_t frame = 128u;
    /* Kept deliberately, and only in this build: a CRC of the bytes as they are
     * handed to the UART, against the CRC in the header that was computed from
     * a first pass over the same RAM.  Equal, and the program sent exactly what
     * it hashed and whatever the board saw wrong was the link's doing; different,
     * and the RAM moved between the two passes, which is a fault of the
     * emulator's state rather than of the transport.  The two are
     * indistinguishable from the board's side -- both look like the right number
     * of wrong bytes -- and this is the cheapest place to tell them apart.  It
     * runs on this board only under `make BINARY=1`, so the default image is
     * unchanged by its presence, and core-log entry 17 is where it earned its
     * keep: equal on the saves that worked, and the failing one's own send line
     * arriving with its first eighteen characters eaten, which is the transport
     * and not the state.  It stays because the next cycle's instrument is this
     * one; it is one line of output per save.  Its own line is printed before
     * DONE, so on a failed save it is inside the bytes the board over-consumes
     * and is lost with them -- a few blank lines after the payload would
     * preserve it if that number is ever wanted from a failure. */
    uint32_t sent = 0;

    printf("Tiny89 NVRAM DATA %lu %08" PRIx32 "\n", (unsigned long)bytes, crc);
    save_arm_pace();

    /* No warm-up of its own, and not through stdio.  After the header every
     * byte is payload, so an empty line would shift the lot; and `_write()`
     * turns each newline into a carriage return and a newline, which in a
     * payload of raw memory would insert a byte and change the length.  The
     * BEGIN lines above are the whole warm-up this shape gets. */
    for (offset = 0; offset < bytes; offset++) {
        const uint8_t byte = tihw.ram[offset];

        sent = t89_save_crc32(sent, &byte, 1);
        t89_put_char((char)byte);
        if ((offset + 1u) % frame == 0u) {
            save_pace();
            /* The board files its buffer at this boundary, so this is where
             * the pause has to cover the card. */
            if ((offset + 1u) % T89_SAVE_CHUNK_BYTES == 0u) {
                save_chunk_pace();
            }
        }
    }
    t89_wait_tx_idle();
    save_pace();
    printf("Tiny89 SAVE hashed %08" PRIx32 " header %08" PRIx32 " %s\n",
           sent, crc,
           sent == crc ? "the bytes sent are the bytes hashed"
                       : "THE RAM MOVED UNDER THE READ");
#else
    printf("Tiny89 NVRAM SAVE %lu %08" PRIx32 "%s\n", (unsigned long)bytes,
           crc, t89_save_is_blank(tihw.ram, bytes) ? " BLANK" : "");
    save_pace();

    /* The payload is a burst of its own, and a burst's opening is where this
     * link loses a chunk -- the BEGIN lines above are the same remedy for the
     * header, which once arrived as "TinyNES N" and armed no save at all.
     * Measured on 2026-10-16: 8096 of 8192 bytes arrived, exactly three lines
     * short, with nothing dropped in the board's link ring, so the loss is a
     * frame that neither end reports.  An empty line decodes to nothing
     * (t89_save_hex writes no bytes for it), so a few of them cost the card
     * nothing and spend the loss on air instead of on the calculator's RAM.
     *
     * What these lines are worth is not settled: the byte payload has since
     * arrived whole with no warm-up possible, so the pace raised in the same
     * breath may be the whole of entry 14's fix (core-log entry 15). */
    for (lead = 0; lead < 4; lead++) {
        printf("\n");
        save_pace();
    }

    /* The payload as hex lines, which is the shape that has produced a valid
     * file in the user's configuration.  It is 65.8 s for 256 KiB rather than
     * 16.6 s, and it is the default because a lost line costs 32 bytes and the
     * rest re-aligns, where the byte payload above is shifted by one. */
    for (offset = 0; offset < bytes; offset += T89_SAVE_LINE_BYTES) {
        t89_save_hex(line, tihw.ram + offset, T89_SAVE_LINE_BYTES);
        printf("%s\n", line);
        save_pace();
    }
#endif

    printf("Tiny89 NVRAM DONE %08" PRIx32 "\n", crc);
    t89_wait_tx_idle();
    save_pace();
    /* SENT, not SAVED: this end can see what it transmitted and nothing else.
     * Whether a save exists is the board's to say, and a cycle where the two
     * disagreed is why the word changed. */
    printf("Tiny89 RAM SENT %lu %08" PRIx32 " %lu ms%s\n", (unsigned long)bytes,
           crc, (unsigned long)((t89_cycles64() - started) /
                                (T89_CORE_HZ / 1000u)),
           t89_save_is_blank(tihw.ram, bytes) ? " BLANK" : "");
}

/*
 * Hand the calculator's flash to the board, when it differs from what this
 * session started with.  It is 4 MiB, so it travels as bytes rather than hex
 * -- 128 to a frame, which is minutes rather than the hours hex would take --
 * and the board checks the length and CRC-32 and throws the file away if they
 * disagree, so a save lost in the link costs the attempt and nothing else.
 * Nothing here runs unless the flash actually changed: an ordinary session
 * that only sets variables writes the 256 KiB RAM and stops.
 */
static void save_flash(void)
{
    const size_t bytes = (size_t)tihw.rom_size;
    const uint32_t crc = t89_save_crc32(0, tihw.rom, bytes);
    size_t offset;
    int lead;

    t89_dcache_wbinval();
    for (lead = 0; lead < 3; lead++) {
        printf("Tiny89 FLASH BEGIN\n");
        save_pace();
    }
    printf("Tiny89 FLASH DATA %lu %08" PRIx32 "\n", (unsigned long)bytes, crc);
    save_arm_pace();
    for (offset = 0; offset < bytes; offset++) {
        t89_put_char((char)tihw.rom[offset]);
        if ((offset + 1u) % 128u == 0u) {
            save_pace();
            if ((offset + 1u) % T89_SAVE_CHUNK_BYTES == 0u) {
                save_chunk_pace();
            }
        }
    }
    t89_wait_tx_idle();
    save_pace();
    printf("Tiny89 FLASH SENT %lu %08" PRIx32 "\n", (unsigned long)bytes, crc);
    fflush(stdout);
}

/*
 * The whole machine, the way TiEmu's own state file keeps it.
 *
 * core/state.c's ti68k_state_save writes six things and ti68k_state_load puts
 * all six back: the register file (`regs`), the I/O ports, RAM, the pieces of
 * Ti68kHardware that are state rather than layout, and the flash blocks that
 * differ from the image.  Restoring RAM alone is not a state: the machine is
 * then still at its reset vector, so the calculator boots -- and a boot that
 * finds no valid system state re-initialises the very thing the save was for.
 * That is why a restored RAM came back with an empty variable list.
 *
 * The flash is not duplicated here: the card already carries the 4 MiB image
 * and it is restored by its own channel, so this blob is registers, ports, RAM
 * and the hardware fields.  The layout itself is t89_state.h's, shared with
 * the browser app so a state written there is read here.
 */

/* The bytes of the payload, paced exactly as the RAM save is: the board files
 * the stream 512 bytes at a time and the pause at each boundary is what keeps
 * arriving bytes out of its masked card writes. */
static void stream_bytes(const uint8_t *data, size_t bytes)
{
    size_t offset;

    for (offset = 0; offset < bytes; offset++) {
        t89_put_char((char)data[offset]);
        if ((offset + 1u) % 128u == 0u) {
            save_pace();
            if ((offset + 1u) % T89_SAVE_CHUNK_BYTES == 0u) {
                save_chunk_pace();
            }
        }
    }
    t89_wait_tx_idle();
    save_pace();
}

/* Hand the board the machine.  On the card it is
 * /root/states/ti89/<image>.state, the channel TinyNES's save states use. */
static void save_state(void)
{
    const size_t bytes = sizeof(state_header_t) + sizeof(regs) +
                         (size_t)tihw.io_size + (size_t)tihw.io2_size +
                         (size_t)tihw.io3_size + (size_t)tihw.ram_size +
                         sizeof(state_extras_t);
    uint8_t *blob;
    state_header_t header;
    state_extras_t extras;
    uint8_t *p;
    uint32_t crc;
    int lead;

    blob = malloc(bytes);
    if (blob == NULL) {
        printf("BAD Tiny89 state allocation\n");
        return;
    }

    memset(&extras, 0, sizeof extras);
    extras.lcd_adr = tihw.lcd_adr;
    extras.contrast = tihw.contrast;
    extras.on_off = tihw.on_off;
    extras.protect = tihw.protect;
    extras.archive_limit = tihw.archive_limit;
    memcpy(extras.ram_exec, tihw.ram_exec, sizeof extras.ram_exec);
    extras.timer_value = tihw.timer_value;
    extras.timer_init = tihw.timer_init;
    extras.rtc_value = tihw.rtc_value;
    extras.pc = m68k_getpc();

    header.magic = T89_STATE_MAGIC;
    header.version = T89_STATE_VERSION;
    header.regs_size = (uint32_t)sizeof(regs);
    header.io_size = (uint32_t)tihw.io_size;
    header.io2_size = (uint32_t)tihw.io2_size;
    header.io3_size = (uint32_t)tihw.io3_size;
    header.ram_size = (uint32_t)tihw.ram_size;
    header.extras_size = (uint32_t)sizeof extras;

    /* The CPU wrote RAM and ports through the D-cache: write them back before
     * they are copied (tinynes.h, and TinyNES does the same before a save). */
    t89_dcache_wbinval();

    p = blob;
    memcpy(p, &header, sizeof header); p += sizeof header;
    memcpy(p, &regs, sizeof regs); p += sizeof regs;
    memcpy(p, tihw.io, (size_t)tihw.io_size); p += tihw.io_size;
    memcpy(p, tihw.io2, (size_t)tihw.io2_size); p += tihw.io2_size;
    memcpy(p, tihw.io3, (size_t)tihw.io3_size); p += tihw.io3_size;
    memcpy(p, tihw.ram, (size_t)tihw.ram_size); p += tihw.ram_size;
    memcpy(p, &extras, sizeof extras);

    crc = t89_save_crc32(0, blob, bytes);

    /* Warm the link, as both other saves do: a burst's opening is where it
     * loses a chunk, and a lost header arms nothing at all. */
    for (lead = 0; lead < 3; lead++) {
        printf("Tiny89 STATE BEGIN\n");
        save_pace();
    }
    printf("Tiny89 STATE DATA %lu %08" PRIx32 "\n", (unsigned long)bytes, crc);
    save_arm_pace();
    stream_bytes(blob, bytes);
    free(blob);

    printf("Tiny89 STATE SENT %lu %08" PRIx32 "\n", (unsigned long)bytes, crc);
    fflush(stdout);
}

/* Put the machine back and let the CPU continue where it left off, which is
 * what makes this a save state rather than a memory image: ti68k_state_load's
 * own ending, m68k_setpc(m68k_getpc()); MakeFromSR(), is the same three lines
 * core/uae/newcpu.c keeps behind its STATE_RESTORE branch.  Returns false when
 * the card has no state for this image, so the caller falls back to RAM. */
static int restore_state(void)
{
    uint8_t *blob = malloc(T89_STATE_MAX);
    const state_header_t *header;
    const uint8_t *p;
    state_extras_t extras;
    int have_extras = 0;
    uint32_t got = 0;
    int tenth;

    if (blob == NULL) {
        printf("BAD Tiny89 state allocation\n");
        return 0;
    }
    printf("Tiny89 STATE WANT %08" PRIx32 " %08" PRIx32 "\n",
           (uint32_t)(uintptr_t)blob, (uint32_t)T89_STATE_MAX);
    fflush(stdout);
    tn_load_reset();
    for (tenth = 0; tenth < 30 && got == 0; tenth++) {
        got = tn_load_poll(TN_CORE_HZ / 10);
    }
    if (got != (uint32_t)(uintptr_t)blob) {
        printf("Tiny89 STATE none yet\n");
        free(blob);
        return 0;
    }

    header = (const state_header_t *)blob;
    if (header->magic != T89_STATE_MAGIC ||
        header->version != T89_STATE_VERSION ||
        header->regs_size != (uint32_t)sizeof(regs) ||
        header->io_size != (uint32_t)tihw.io_size ||
        header->io2_size != (uint32_t)tihw.io2_size ||
        header->io3_size != (uint32_t)tihw.io3_size ||
        header->ram_size != (uint32_t)tihw.ram_size) {
        /* A state from another build or another calculator: RAM is still
         * worth having, and the caller asks for it next. */
        printf("Tiny89 STATE mismatch\n");
        free(blob);
        return 0;
    }

    p = blob + sizeof *header;
    memcpy(&regs, p, sizeof regs); p += sizeof regs;
    memcpy(tihw.io, p, (size_t)tihw.io_size); p += tihw.io_size;
    memcpy(tihw.io2, p, (size_t)tihw.io2_size); p += tihw.io2_size;
    memcpy(tihw.io3, p, (size_t)tihw.io3_size); p += tihw.io3_size;
    memcpy(tihw.ram, p, (size_t)tihw.ram_size); p += tihw.ram_size;
    if (header->extras_size == (uint32_t)sizeof extras) {
        memcpy(&extras, p, sizeof extras);
        tihw.lcd_adr = extras.lcd_adr;
        tihw.contrast = extras.contrast;
        tihw.on_off = extras.on_off;
        tihw.protect = extras.protect;
        tihw.archive_limit = extras.archive_limit;
        memcpy(tihw.ram_exec, extras.ram_exec, sizeof tihw.ram_exec);
        tihw.timer_value = extras.timer_value;
        tihw.timer_init = extras.timer_init;
        tihw.rtc_value = extras.rtc_value;
        have_extras = 1;
    }
    free(blob);

    /* Resume where the machine was, not where its last setpc left it.  This is
     * ti68k_state_load's own ending (core/state.c): the saved PC, then the
     * status register's flags and interrupt mask.  The stack pointer needs no
     * help -- regs.regs[15] came back with the rest of the register file, and
     * MakeFromSR only swaps usp/isp when the privilege bit moved, which it did
     * not.  (core/uae/newcpu.c keeps an isp/usp assignment behind #if 0; it is
     * disabled because it overwrites a live SP with a stale one.) */
    m68k_setpc(have_extras ? extras.pc : regs.pc);
    MakeFromSR();
    printf("Tiny89 STATE resumed PC=%08" PRIx32 "\n", (uint32_t)regs.pc);
    return 1;
}

#ifdef T89_DIAG
/*
 * What the two memory images do across the OS's boot, which is the question a
 * restored RAM left open: the calculator ran its first-boot installation
 * instead of resuming a session, and either the OS re-initialises the RAM
 * whatever we restore, or it keeps the RAM and the flash is the missing half --
 * this emulator rebuilds the flash from the converted image at every start
 * (core/ti_hw/mem.c copies img->data into tihw.rom), so the calculator is
 * handed a virgin flash every time.
 *
 * A pair of CRC-32s at both ends of the boot tells the two apart: a RAM CRC
 * that changes means the OS writes over what we restored, and a RAM CRC that
 * holds while the flash CRC moves means the flash is the whole story.  Printed
 * once before the CPU first runs and then at READY and on every heartbeat, so
 * the readings also say when it happened.  DIAG=1 only: the default image says
 * nothing, which is entry 10's rule.
 */
static void diag_line(const char *when)
{
    printf("Tiny89 DIAG %s RAM=%08" PRIx32 " FLASH=%08" PRIx32 "\n", when,
           t89_save_crc32(0, tihw.ram, (size_t)tihw.ram_size),
           t89_save_crc32(0, tihw.rom, (size_t)tihw.rom_size));
}

/* A CRC says the RAM changed, not whether it was rewritten: the calculator
 * updates its own RAM every second, so any two readings differ.  What a cold
 * boot looks like is the restored bytes *gone*, so this keeps the image as it
 * arrived and counts, at READY, how much of it is still there and how much of
 * the memory is now zero or 0xFF -- the two patterns a memory clear leaves. */
static uint8_t *s_ram_before;

static void diag_snapshot(void)
{
    const size_t bytes = (size_t)tihw.ram_size;

    if (s_ram_before != NULL) {
        return;
    }
    s_ram_before = malloc(bytes);
    if (s_ram_before != NULL) {
        memcpy(s_ram_before, tihw.ram, bytes);
    }
}

static void diag_survival(const char *when)
{
    const size_t bytes = (size_t)tihw.ram_size;
    size_t same = 0;
    size_t zeros = 0;
    size_t ones = 0;

    if (s_ram_before == NULL) {
        printf("Tiny89 DIAG SURVIVAL %s no snapshot\n", when);
        return;
    }
    t89_dcache_wbinval();
    for (size_t i = 0; i < bytes; i++) {
        if (tihw.ram[i] == s_ram_before[i]) {
            same++;
        }
        if (tihw.ram[i] == 0x00) {
            zeros++;
        } else if (tihw.ram[i] == 0xff) {
            ones++;
        }
    }
    printf("Tiny89 DIAG SURVIVAL %s %lu of %lu as restored, %lu zero, %lu ff\n",
           when, (unsigned long)same, (unsigned long)bytes,
           (unsigned long)zeros, (unsigned long)ones);
}
#endif

/*
 * The board's clock, asked for once while the boot ROM's loader still owns the
 * link -- the same window the BIOS and the saved state arrive in, and the same
 * request shape.  TinyTang answers with a six-byte blob (its `Tiny89 SETTINGS
 * WANT`): UTC seconds since 1970, little-endian, then the desktop's zone in
 * minutes east of UTC, little-endian and signed.  Zero seconds is the board's
 * "the RTC has never been set", which seeds nothing.
 *
 * The calculator has no clock chip in this fabric, so this is the only date it
 * ever gets; from here it counts the seconds itself, which is why one blob per
 * launch is enough and why nothing has to cross the link while it runs.
 */
static void request_clock(void)
{
    /* In BSS, not on the stack: the boot ROM's loader only writes into the
     * AE350's SDRAM (0x40000000 and up), and a stack buffer here lives at
     * 0x6000fea4 -- outside it -- so the loader NAKs the header with an
     * address reason and nothing arrives.  Every other request this program
     * makes names a SDRAM buffer for the same reason. */
    static uint8_t blob[6];
    uint32_t got = 0;
    uint32_t utc;
    int32_t zone_min;
    int tenth;

    memset(blob, 0, sizeof blob);
    printf("Tiny89 SETTINGS WANT %08lx %08lx\n",
           (unsigned long)(uint32_t)(uintptr_t)blob,
           (unsigned long)(uint32_t)sizeof blob);
    fflush(stdout);
    tn_load_reset();
    for (tenth = 0; tenth < 20 && got == 0; tenth++) {
        got = tn_load_poll(TN_CORE_HZ / 10);
    }
    if (got != (uint32_t)(uintptr_t)blob) {
        printf("Tiny89 CLOCK none\n");
        return;
    }

    utc = (uint32_t)blob[0] | ((uint32_t)blob[1] << 8) |
          ((uint32_t)blob[2] << 16) | ((uint32_t)blob[3] << 24);
    zone_min = (int32_t)(int16_t)((uint16_t)blob[4] | ((uint16_t)blob[5] << 8));
    if (t89_rtc_seed(utc, zone_min, t89_cycles64())) {
        printf("Tiny89 CLOCK %lu %ld\n", (unsigned long)utc, (long)zone_min);
    } else {
        printf("Tiny89 CLOCK not set\n");
    }
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
#ifdef T89_AUTOSAVE
    int s_autosaved = 0;
#endif
#ifdef T89_LINK_TRACE
    uint32_t last_link_report = 0;
#endif
#ifdef T89_LINKSEND
    uint32_t s_link_tries = 0;
#endif

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

    printf("Tiny89 WANT %08lx %08lx\n", (unsigned long)(uintptr_t)upgrade,
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
    /* The link port's wires and byte registers become real here; without this
     * the calculator's Link app drives stubs and no byte can cross. */
    t89_link_install();
    printf("Tiny89 BOOT SSP=%08" PRIx32 " PC=%08" PRIx32 "\n",
        tihw.initial_ssp, tihw.initial_pc);

    /*
     * The whole machine first: if a state came back, the calculator resumes
     * where it was saved and never runs its boot at all.  That is the
     * difference between a save state and a RAM image -- a machine left at its
     * reset vector boots, and a boot rebuilds the state the save was for.
     */
    if (restore_state()) {
        printf("Tiny89 RESUMED\n");
    } else {
    /*
     * Otherwise the calculator's RAM, if a previous session left one on the
     * card.  The board serves a save headerless straight into the buffer named
     * here and the loader's own block checksums judge it, so the file lands in
     * tihw.ram the way a battery-backed calculator's RAM is found at power-up.
     * A first boot has no file, ae350run says so, and the zeroed RAM TiEmu
     * allocated is what the OS then cold-starts on.
     *
     * The request goes out now, while the UART is still in loader mode: once
     * TinyNES LINK is printed the line carries text frames and cannot ask for
     * a file any more.
     */
    printf("Tiny89 NVRAM WANT %08" PRIx32 " %08" PRIx32 "\n",
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
    }
    /*
     * The file the board was asked to hand the calculator: asked for here,
     * while the UART is still in loader mode, and sent later once the machine
     * is running and the calculator's Link screen is listening.
     */
    s_send_blob = malloc(T89_SEND_MAX);
    if (s_send_blob != NULL) {
        uint32_t got = 0;
        int tenth;

        printf("Tiny89 SEND WANT %08" PRIx32 " %08" PRIx32 "\n",
               (uint32_t)(uintptr_t)s_send_blob, (uint32_t)T89_SEND_MAX);
        fflush(stdout);
        tn_load_reset();
        for (tenth = 0; tenth < 30 && got == 0; tenth++) {
            got = tn_load_poll(TN_CORE_HZ / 10);
        }
        if (got == (uint32_t)(uintptr_t)s_send_blob) {
            if (t89_file_parse(s_send_blob, T89_SEND_MAX, &s_send_var)) {
                printf("Tiny89 SEND ready %s type %02x size %u\n",
                       s_send_var.name, (unsigned)s_send_var.type,
                       s_send_var.size);
            } else {
                printf("Tiny89 SEND not a calculator file\n");
                s_send_var.size = 0;
            }
        } else {
            printf("Tiny89 SEND none\n");
        }
    }

    /*
     * The calculator's flash, if a previous session left one.  The OS keeps
     * its installation and its apps there, and a freshly converted flash
     * under a restored RAM is what sends the calculator into its first-boot
     * installation; the two halves have to come back together, so the flash
     * is asked for by the same protocol into TiEmu's own rom buffer.
     */
    printf("Tiny89 FLASH WANT %08" PRIx32 " %08" PRIx32 "\n",
           (uint32_t)(uintptr_t)tihw.rom, (uint32_t)tihw.rom_size);
    fflush(stdout);
    tn_load_reset();
    {
        uint32_t got = 0;
        int tenth;

        for (tenth = 0; tenth < 300 && got == 0; tenth++) {
            got = tn_load_poll(TN_CORE_HZ / 10);
        }
        if (got == (uint32_t)(uintptr_t)tihw.rom) {
            printf("Tiny89 FLASH GOT %lu %08" PRIx32 "\n",
                   (unsigned long)tihw.rom_size,
                   t89_save_crc32(0, tihw.rom, (size_t)tihw.rom_size));
        } else {
            printf("Tiny89 FLASH none yet\n");
        }
    }
    s_flash_boot_crc = t89_save_crc32(0, tihw.rom, (size_t)tihw.rom_size);

#ifdef T89_DIAG
    /* The reading before the calculator's own code has run at all: whatever
     * the save put there, and the flash as the converter built it. */
    diag_line("BEFORE");
    diag_snapshot();
#endif

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

    /* The board's clock, asked for while the boot ROM's loader still owns the
     * link -- it answers over the same path the BIOS and the saved state come
     * in on, so this has to happen before the link changes hands below.  Its
     * absence is not a fault: the calculator then keeps the platform's count
     * and says so. */
    t89_rtc_reset();
    request_clock();

    /* The keyboard arrives as TinyTang's frames and the fabric's desk link
     * parses them, so the UART has to be in its link mode.  A write to CTRL
     * completes once everything written before it has gone out, so the line
     * below reaches the host as text first; the host loader watches for
     * exactly that line to start reading frames instead of text (TinyNES's
     * tools/load_ae350.py, which is also Tiny89's).  Nothing is printed after
     * this point but frames. */
    printf("Tiny89 LINK\n");
    T89_UART_CTRL = T89_UART_LINK_MODE;
    printf("Tiny89 KBD seq=%u\n", (unsigned)(T89_KBD_HI >> 24));

    last_frames = T89_VIDEO_FRAMES;

    for (;;) {
        uint32_t hash;
        int result, count;

        /* Hand the calculator the file it was given: every two seconds until
         * it takes it, so being slow to reach the Link screen costs nothing,
         * and no more than half a minute of trying. */
        if (!s_send_done && s_send_var.size != 0 && ready_printed) {
            s_send_ticks++;
        }
        if (!s_send_done && s_send_var.size != 0 && ready_printed &&
            s_send_tries < 1u && s_send_ticks % 120u == 0u) {
            s_send_last_try = presented;
            s_send_tries++;
            printf("Tiny89 SEND try %u: %s type %02x size %u\n",
                   s_send_tries, s_send_var.name, (unsigned)s_send_var.type,
                   s_send_var.size);
            /* A flash app is an archived variable, and libticalcs' send_var
             * maps ATTRB_ARCHIVED to type 0x27 before sending; offering 0x24
             * is "an app that is not archived", which the calculator refuses
             * with a link-transmission error. */
            const uint8_t type =
                (s_send_var.type == 0x24u || s_send_var.type == 0x1Cu)
                    ? 0x27u : s_send_var.type;
            if (t89_proto_send_var(type, s_send_var.name,
                                   s_send_var.data, s_send_var.size)) {
                s_send_done = 1;
                printf("Tiny89 SEND done\n");
            }
        }


        /* (a) One frame of emulated time.  TiEmu refreshes its timers,
         * hardware and keyboard inside hw_m68k_run(), so nothing else is
         * stepped here. */
        result = hw_m68k_run(FRAME_INSTRUCTIONS, FRAME_CYCLES);
        if (result) {
            printf("BAD Tiny89 CPU result=%d\n", result);
            return 1;
        }

#ifdef T89_LINKSEND
        /* Diagnostic: every ten seconds, offer the calculator a variable.
         * Nothing happens unless it is sitting in Receive mode, and an
         * unanswered offer costs one line of output, so the user can walk to
         * the Link screen at any pace. */
        if (ready_printed && presented / 600u != s_link_tries) {
            static const uint8_t body[3] = { 0x00, 0x00, 0x00 };
            s_link_tries = presented / 600u;
            printf("Tiny89 PROTO attempt %u\n", (unsigned)s_link_tries);
            (void)t89_proto_send_var(0x05, "ZZZ", body, sizeof body);
        }
#endif

#ifdef T89_LINK_TRACE
        /* Diagnostic: every byte the calculator puts on the link, and a
         * periodic line saying whether it has touched the port at all. */
        for (int b = t89_link_read(); b >= 0; b = t89_link_read()) {
            printf("Tiny89 LINK rx %02x\n", (unsigned)b);
        }
        if (presented != 0 && presented % 300 == 0 && presented != last_link_report) {
            last_link_report = presented;
            t89_link_report("now");
        }
#endif

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
            /* The state carries the RAM, the registers, the ports and the
             * hardware fields: the .ram file is the fallback for a card that
             * has no state yet, not something to rewrite on every save. */
            save_state();
            if (t89_save_crc32(0, tihw.rom, (size_t)tihw.rom_size) !=
                s_flash_boot_crc) {
                save_flash();
                s_flash_boot_crc = t89_save_crc32(0, tihw.rom,
                                                  (size_t)tihw.rom_size);
            }
        }

        /* (b3) Control and Alt with q ends the session and returns, so the
         * firmware that started it can put the desktop core back.  The result
         * word is the clean return the loader counts as a run. */
        if (command == T89_KBD_CMD_EXIT) {
            printf("Tiny89 EXIT\n");
            fflush(stdout);
            return 0;
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

#ifdef T89_AUTOSAVE
            /* Diagnostic only: save without a keypress, so the save path can
             * be exercised from the console.  Not in any shipped build. */
            if (!s_autosaved && ready_printed && presented >= 300) {
                s_autosaved = 1;
                printf("Tiny89 AUTOSAVE at frame %u\n", (unsigned)presented);
                save_state();
                printf("Tiny89 AUTOSAVED\n");
                /* Leave the machine so the next launch is the one that
                 * restores: a test of the whole cycle needs no keypress and
                 * no power cycle between the two halves. */
                printf("Tiny89 EXIT 0\n");
                fflush(stdout);
                return 0;
            }
#endif

            /* (d) The deployment tool waits for this string, so it is printed
             * exactly once, at whichever of the two conditions comes first. */
            if (!ready_printed &&
                (presented >= READY_FRAMES || stable >= READY_STABLE_FRAMES)) {
                ready_printed = 1;
                printf("Tiny89 READY PC=%08" PRIx32 " LCD=%08" PRIx32 "\n",
                    (uint32_t)m68k_getpc(), hash);
#ifdef T89_DIAG
                diag_line("READY");
                diag_survival("READY");
#endif
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
#ifdef T89_DIAG
                diag_line("FRAME");
#endif
            }
        }
    }
}
