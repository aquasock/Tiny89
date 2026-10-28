/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Tiny89 in a browser: the same TiEmu core the board runs, with a page in
 * place of the AE350.
 *
 * The point is not to look at a calculator.  It is to produce the two files
 * the board already knows how to load -- the 4 MiB flash image it asks for
 * with `Tiny89 FLASH WANT` and the 256 KiB RAM it asks for with
 * `Tiny89 NVRAM WANT` -- with the user's own software already installed in
 * them, so nothing has to cross the board's link at run time.
 *
 * Installing is done the only way that cannot be wrong: the calculator's own
 * operating system does it.  This module boots the user's `.89u`, then hands
 * each file to `t89_proto_send_var()`, which drives the emulated link port
 * (`t89_link.c`) and pumps the emulated CPU while it waits, exactly as the
 * board's program would.  Whatever the OS does with the file -- variable,
 * program, or the certificate check a Flash App goes through -- it does here.
 * Nothing in this file understands TI's filesystem, and nothing needs to.
 *
 * Not covered yet: `**TIFL**` Flash Apps.  They install through a different
 * exchange from a variable's (`send_app` in libticalcs, not `send_var`), so a
 * `.89k` that is a TIFL container is reported as unsupported rather than sent
 * as the wrong thing.
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "gscales.h"
#include "hw.h"
#include "images.h"
#include "libuae.h"
#include "m68k.h"
#include "ti68k_def.h"

#include "t89_bios.h"
#include "t89_file.h"
#include "t89_lcd.h"
#include "t89_link.h"
#include "t89_proto.h"
#include "t89_state.h"

#ifdef __EMSCRIPTEN__
#include <emscripten/emscripten.h>
#define T89_EXPORT EMSCRIPTEN_KEEPALIVE
#else
#define T89_EXPORT
#endif

#define T89_FLASH_BYTES (4u * 1024u * 1024u)
/* The calculator's visible LCD: 30 rows of 128 bytes, the region the board's
 * program hashes for its heartbeat and the host probe for its RESULT line. */
#define T89_LCD_BYTES (30u * 128u)

static int s_booted;
static unsigned s_lcd_hash;
static unsigned s_lcd_first;
static unsigned s_lcd_changes;
static char s_error[160];
static t89_var_t s_last_var;
/* One 160x100 image, four levels, for the page's canvas.  The page repaints it
 * from this buffer rather than keeping its own copy. */
static uint8_t s_image[T89_LCD_ROW_BYTES * T89_LCD_HEIGHT];

static unsigned fnv1a(const uint8_t *data, size_t length)
{
    uint32_t hash = UINT32_C(2166136261);
    size_t i;

    for (i = 0; i < length; i++) {
        hash ^= data[i];
        hash *= UINT32_C(16777619);
    }
    return hash;
}

/* Hash what the calculator is showing, the way the board's program does. */
static void update_lcd(void)
{
    tihw.lcd_ptr = (char *)&tihw.ram[tihw.lcd_adr & (tihw.ram_size - 1)];
    s_lcd_hash = fnv1a((const uint8_t *)tihw.lcd_ptr, T89_LCD_BYTES);
    if (s_lcd_first == 0u) {
        s_lcd_first = s_lcd_hash;
    } else if (s_lcd_hash != s_lcd_first) {
        s_lcd_changes++;
    }
}

T89_EXPORT const char *t89_error(void)
{
    return s_error;
}

/*
 * Convert the user's upgrade file and bring the machine up on it.  Returns 0,
 * or a negative code with t89_error() set.  The flash buffer is handed to the
 * core, which copies it into its own ROM and frees it (mem.c), so it must be
 * heap memory and must not be touched afterwards.
 */
T89_EXPORT int t89_boot(const uint8_t *upgrade, int length)
{
    uint8_t *flash;

    s_error[0] = '\0';
    if (s_booted) {
        hw_exit();
        s_booted = 0;
    }
    if (upgrade == NULL || length < 16 || (unsigned)length > T89_BIOS_MAX) {
        snprintf(s_error, sizeof s_error,
                 "not an upgrade file: %d bytes", length);
        return -1;
    }

    flash = malloc(T89_FLASH_BYTES);
    if (flash == NULL) {
        snprintf(s_error, sizeof s_error, "no memory for a 4 MiB image");
        return -2;
    }
    if (t89_bios_from_89u(upgrade, (size_t)length, flash, s_error,
                          sizeof s_error) != 0) {
        free(flash);
        return -3;
    }

    memset(&img_infos, 0, sizeof(img_infos));
    img_infos.data = (char *)flash;
    img_infos.calc_type = TI89t;
    memcpy(img_infos.version, "3.10", 5);
    img_infos.flash = FLASH_ROM;
    img_infos.has_boot = 0;
    img_infos.size = T89_FLASH_BYTES;
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
        snprintf(s_error, sizeof s_error, "the emulated hardware did not start");
        return -4;
    }
    /* The link port's wires and byte registers become real here; without this
     * the calculator's Link app drives stubs and no byte can cross. */
    t89_link_install();
    /* Seed TiEmu's gray-plane model the way the GTK frontend did after a reset
     * (gui/calc/calc.c:587-590): plane 0 is the LCD at tihw.lcd_adr, its
     * buffer is the calculator's RAM there, and one plane is in use.  Without
     * it lcd_planebufs[0] stays NULL until sixteen plane scans have gone by and
     * the picture reads as blank paper. */
    lcd_planes[0] = tihw.lcd_adr;
    lcd_planebufs[0] = &tihw.ram[tihw.lcd_adr];
    ngc = 1;
    lcd_changed = 1;
    s_booted = 1;
    s_lcd_hash = s_lcd_first = s_lcd_changes = 0u;
    update_lcd();
    return 0;
}

/* Run the machine.  Non-zero means the CPU stopped on an exception. */
T89_EXPORT int t89_run(int instructions)
{
    int result;

    if (!s_booted) {
        return -1;
    }
    result = hw_m68k_run((int)instructions, 0);
    update_lcd();
    return result;
}

T89_EXPORT unsigned t89_pc(void)
{
    return (unsigned)m68k_getpc();
}

T89_EXPORT unsigned t89_cycles(void)
{
    return s_booted ? (unsigned)hw_m68k_get_cycle_count(0) : 0u;
}

T89_EXPORT unsigned t89_lcd_hash(void) { return s_lcd_hash; }
T89_EXPORT unsigned t89_lcd_changes(void) { return s_lcd_changes; }
T89_EXPORT int t89_on(void) { return s_booted ? tihw.on_off : 0; }

/* The picture, rendered from TiEmu's plane state by the board's own module. */
T89_EXPORT unsigned t89_lcd_image(void)
{
    if (!s_booted) {
        return 0u;
    }
    memset(s_image, 0, sizeof s_image);
    t89_lcd_render(s_image);
    return (unsigned)(uintptr_t)s_image;
}

T89_EXPORT int t89_lcd_width(void) { return (int)T89_LCD_WIDTH; }
T89_EXPORT int t89_lcd_height(void) { return (int)T89_LCD_HEIGHT; }
T89_EXPORT int t89_lcd_stride(void) { return (int)T89_LCD_ROW_BYTES; }
T89_EXPORT const char *t89_lcd_mode_text(void) { return t89_lcd_mode(); }

T89_EXPORT unsigned t89_flash_ptr(void) { return (unsigned)(uintptr_t)tihw.rom; }
T89_EXPORT unsigned t89_flash_size(void) { return (unsigned)tihw.rom_size; }
T89_EXPORT unsigned t89_ram_ptr(void) { return (unsigned)(uintptr_t)tihw.ram; }
T89_EXPORT unsigned t89_ram_size(void) { return (unsigned)tihw.ram_size; }

/* The name and type of the file the last install looked at, for the page's
 * table.  A variable's name is eight characters and may not be printable, so
 * it is copied out rather than pointed at. */
T89_EXPORT const char *t89_last_name(void) { return s_last_var.name; }
T89_EXPORT int t89_last_type(void) { return (int)s_last_var.type; }
T89_EXPORT unsigned t89_last_size(void) { return s_last_var.size; }

/*
 * Install one file into the running calculator.
 *
 *   0   the calculator took it
 *  -1   not a calculator container this build understands
 *  -2   a Flash App (`**TIFL**`): the app exchange is not implemented yet
 *  -3   the calculator refused the exchange (see t89_proto_send_var)
 *
 * The caller runs the machine between calls so the OS can settle; the sender
 * itself pumps the CPU while it waits for each answer.
 */
T89_EXPORT int t89_install(const uint8_t *buffer, int length)
{
    if (!s_booted) {
        snprintf(s_error, sizeof s_error, "the machine is not running");
        return -1;
    }
    /* t89_file_parse() answers non-zero when it understood the container and
     * the payload's sum agreed (t89_file.h), so zero is the failure here. */
    if (buffer == NULL || length <= 0 ||
        t89_file_parse(buffer, (unsigned)length, &s_last_var) == 0) {
        snprintf(s_error, sizeof s_error,
                 "not a calculator file this build can read");
        return -1;
    }
    if (length >= 8 && memcmp(buffer, "**TIFL**", 8) == 0) {
        snprintf(s_error, sizeof s_error,
                 "%s is a Flash App: that exchange is not implemented yet",
                 s_last_var.name);
        return -2;
    }
    if (t89_proto_send_var(s_last_var.type, s_last_var.name, s_last_var.data,
                           s_last_var.size) == 0) {
        snprintf(s_error, sizeof s_error,
                 "%s was refused by the calculator", s_last_var.name);
        return -3;
    }
    s_error[0] = '\0';
    update_lcd();
    return 0;
}


/*
 * The whole machine, in the layout t89_state.h defines and the board's program
 * already restores.  This is what the page should hand the card: a RAM image
 * alone cannot survive, because the operating system rebuilds its RAM at every
 * cold boot and takes the user's variables with it (core-log entry 22).  A
 * state is resumed instead of booted, so nothing is rebuilt.
 */
T89_EXPORT unsigned t89_state_size(void)
{
    if (!s_booted) {
        return 0u;
    }
    return (unsigned)(sizeof(state_header_t) + sizeof(regs) +
                      (size_t)tihw.io_size + (size_t)tihw.io2_size +
                      (size_t)tihw.io3_size + (size_t)tihw.ram_size +
                      sizeof(state_extras_t));
}

T89_EXPORT int t89_state_build(uint8_t *out, unsigned room)
{
    state_header_t header;
    state_extras_t extras;
    const unsigned total = t89_state_size();
    uint8_t *p = out;

    if (!s_booted || out == NULL || total == 0u || room < total) {
        return -1;
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
    extras.pc = (uint32_t)m68k_getpc();

    header.magic = T89_STATE_MAGIC;
    header.version = T89_STATE_VERSION;
    header.regs_size = (uint32_t)sizeof(regs);
    header.io_size = (uint32_t)tihw.io_size;
    header.io2_size = (uint32_t)tihw.io2_size;
    header.io3_size = (uint32_t)tihw.io3_size;
    header.ram_size = (uint32_t)tihw.ram_size;
    header.extras_size = (uint32_t)sizeof extras;

    memcpy(p, &header, sizeof header); p += sizeof header;
    memcpy(p, &regs, sizeof regs); p += sizeof regs;
    memcpy(p, tihw.io, (size_t)tihw.io_size); p += tihw.io_size;
    memcpy(p, tihw.io2, (size_t)tihw.io2_size); p += tihw.io2_size;
    memcpy(p, tihw.io3, (size_t)tihw.io3_size); p += tihw.io3_size;
    memcpy(p, tihw.ram, (size_t)tihw.ram_size); p += tihw.ram_size;
    memcpy(p, &extras, sizeof extras);
    return 0;
}

/* Put a machine back, ending exactly as the board's restore ends: the saved
 * PC, then the status register.  Used by the host test to prove that a state
 * built here is a state this code can resume. */
T89_EXPORT int t89_state_restore(const uint8_t *blob, unsigned length)
{
    const state_header_t *header;
    state_extras_t extras;
    const uint8_t *p;

    if (!s_booted || blob == NULL || length < sizeof *header) {
        return -1;
    }
    header = (const state_header_t *)blob;
    if (header->magic != T89_STATE_MAGIC ||
        header->version != T89_STATE_VERSION ||
        header->regs_size != (uint32_t)sizeof(regs) ||
        header->io_size != (uint32_t)tihw.io_size ||
        header->io2_size != (uint32_t)tihw.io2_size ||
        header->io3_size != (uint32_t)tihw.io3_size ||
        header->ram_size != (uint32_t)tihw.ram_size ||
        header->extras_size != (uint32_t)sizeof extras) {
        return -2;
    }
    if (length < t89_state_size()) {
        return -3;
    }

    p = blob + sizeof *header;
    memcpy(&regs, p, sizeof regs); p += sizeof regs;
    memcpy(tihw.io, p, (size_t)tihw.io_size); p += tihw.io_size;
    memcpy(tihw.io2, p, (size_t)tihw.io2_size); p += tihw.io2_size;
    memcpy(tihw.io3, p, (size_t)tihw.io3_size); p += tihw.io3_size;
    memcpy(tihw.ram, p, (size_t)tihw.ram_size); p += tihw.ram_size;
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

    m68k_setpc(extras.pc);
    MakeFromSR();
    update_lcd();
    return 0;
}

T89_EXPORT void t89_shutdown(void)
{
    if (s_booted) {
        hw_exit();
        s_booted = 0;
    }
}
