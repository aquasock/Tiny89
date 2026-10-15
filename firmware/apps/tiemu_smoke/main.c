/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Tiny89 on the inherited TinyNES ae350_soc fabric: run the emulated TI-89
 * Titanium continuously, presenting the calculator's LCD at the platform's
 * vertical blank and feeding its keypad from a Wii Classic Controller.
 */
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "hw.h"
#include "images.h"
#include "kbd.h"
#include "libuae.h"
#include "m68k.h"
#include "t89_pad.h"
#include "tiny89_platform.h"
#include "ti68k_def.h"

#define FLASH_BYTES (4u * 1024u * 1024u)

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
#define HEARTBEAT_FRAMES    60u  /* one status line per this many frames */
#define MAX_KEY_EVENTS      32

extern const uint8_t tiny89_flash_image[];
extern const uint8_t tiny89_flash_image_end[];

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
 * Sample controller 0 and hand the difference to TiEmu's keypad.
 *
 * This runs every frame rather than only when the fabric's reads-completed
 * field moves.  That field advances on a successful transfer only
 * (wii_pads.sv:451); when a controller stops answering -- unplugged, asleep,
 * out of range -- the fabric clears present and classic and counts a failure
 * instead (:433-438), leaving reads frozen at its last value and DATA holding
 * the last good sample.  Gating on reads therefore misses exactly the case
 * that matters, and would leave the keys of that last sample pressed with no
 * controller present.  Three uncached register reads and one diff cost nothing
 * beside a frame of interpreted M68K.
 *
 * A controller that is absent, or that answers without identifying as a
 * Classic Controller, is presented as the centred, buttonless pad
 * t89_pad_none() returns, which releases anything the module had applied.  It
 * is deliberately not a zeroed t89_pad_t: axis 0 is the stick's minimum and
 * would read as up-left held.
 *
 * With no pad unit at all (see the ID probe in main()) this is never called:
 * the whole session runs with no input.
 *
 * Returns the number of key events applied, or a negative TiEmu/module error.
 */
static int poll_pad(int have_pad, t89_keymap_t *map, t89_pad_t *pad,
                    t89_key_event_t *events)
{
    uint32_t status, lo, hi;
    int count, i;

    if (!have_pad) return 0;

    status = T89_PAD_STATUS(0);
    if ((status & T89_PAD_PRESENT) && (status & T89_PAD_CLASSIC)) {
        lo = T89_PAD_DATA(0);  /* reading DATA also latches DATA2 */
        hi = T89_PAD_DATA2(0) & 0xffffu;
        t89_pad_decode(lo, hi, pad);
    } else {
        t89_pad_none(pad);
    }

    count = t89_keymap_update(map, pad, events, MAX_KEY_EVENTS);
    if (count < 0) return count;

    for (i = 0; i < count; i++) {
        if (events[i].key < 0 || events[i].key >= NB_MAX_KEYS) return -1;
        ti68k_kbd_set_key(events[i].key, events[i].active);
    }
    return count;
}

int main(void)
{
    uint8_t *flash;
    t89_keymap_t map;
    t89_key_event_t events[MAX_KEY_EVENTS];
    t89_pad_t pad;
    const char *layer;
    uint32_t pad_id, frames, last_frames, last_lcd = 0;
    uint32_t presented = 0, stable = 0;
    unsigned buffer = 0;
    int have_pad, ready_printed = 0;

    printf("Tiny89 AE350 TiEmu smoke start\n");
    if ((size_t)(tiny89_flash_image_end - tiny89_flash_image) != FLASH_BYTES) {
        printf("BAD Tiny89 flash size=%" PRIu32 "\n",
            (uint32_t)(tiny89_flash_image_end - tiny89_flash_image));
        return 1;
    }
    flash = malloc(FLASH_BYTES);
    if (!flash) {
        printf("BAD Tiny89 flash allocation\n");
        return 1;
    }
    memcpy(flash, tiny89_flash_image, FLASH_BYTES);

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

    /* Start showing the calculator's LCD: palette first, then the content of
     * buffer 0, written back before the fabric is told to show it. */
    T89_VIDEO_PALETTE(0) = 0x9fb58fu;
    T89_VIDEO_PALETTE(1) = 0x182018u;
    render_lcd(T89_VIDEO_FB(buffer));
    t89_dcache_wbinval();
    T89_VIDEO_CTRL = T89_VIDEO_ENABLE;

    /*
     * Probe the Wii pad unit once.  The image on the Tang is a copy of a
     * TinyNES SoC build and it is not verified that it decodes 0xe8040000, so
     * a missing pad unit must never fail the boot: the session simply runs
     * with no input.
     */
    t89_keymap_reset(&map);
    t89_pad_none(&pad);
    pad_id = T89_PAD_ID;
    have_pad = (pad_id == T89_PAD_ID_VALUE);
    if (have_pad) {
        T89_PAD_CTRL = T89_PAD_QUARTER_100K;
        printf("Tiny89 PAD OK ID=544e5031\n");
    } else {
        printf("Tiny89 PAD NONE ID=%08" PRIx32 "\n", pad_id);
    }

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

        /* (b) Take controller input. */
        count = poll_pad(have_pad, &map, &pad, events);
        if (count < 0) {
            printf("BAD Tiny89 pad result=%d\n", count);
            return 1;
        }

        /* (c) Present at the fabric's vertical blank: refresh the buffer that
         * is not being shown, write the D-cache back, then hand it over by
         * flipping CTRL's buffer bit. */
        frames = T89_VIDEO_FRAMES;
        if (frames != last_frames) {
            last_frames = frames;
            buffer ^= 1u;
            render_lcd(T89_VIDEO_FB(buffer));
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

            /* (e) One heartbeat per HEARTBEAT_FRAMES presented frames. */
            if (presented % HEARTBEAT_FRAMES == 0u) {
                layer = t89_layer_name(map.layer);
                printf("Tiny89 FRAME n=%u PC=%08" PRIx32 " CYC=%u"
                       " LCD=%08" PRIx32 " VBL=%u PAD=%04x KEYS=%d"
                       " LAYER=%s\n",
                    (unsigned)presented, (uint32_t)m68k_getpc(),
                    (unsigned)hw_m68k_get_cycle_count(0), hash,
                    (unsigned)frames, (unsigned)(pad.buttons & 0xffffu),
                    map.applied_count, layer ? layer : "?");
            }
        }
    }
}
