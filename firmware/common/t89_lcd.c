/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Tiny89 grayscale LCD: reads TiEmu's gray-plane state and writes the
 * calculator's 160x100 LCD as four levels, one byte a pixel, at
 * byte offset 256*y + x.  The new Tiny89 video unit scans that out; the
 * app owns the buffer and the SDRAM.
 *
 * Where this comes from (third_party/tiemu, pinned commit):
 *   core/ti_hw/gscales.c : lcd_planebufs[3] (the plane bitmaps, 3840 bytes
 *                          each = 128 rows x 30 bytes), lcd_planes[],
 *                          ngc, lcd_changed, process_address()
 *   core/ti_hw/m68k.c:263 : lcd_hook_hw2(0) from the run loop, for HW2/HW3
 *   gui/calc/screen.c    : gp_seq[9][8], hid_update_lcd() -- the composition
 *                          and grayscales[] shading that the GTK frontend did
 *                          and that Tiny89 dropped
 * The composition itself lives in t89_lcd_gray.c; this file is the binding to
 * TiEmu's globals.  No TiEmu state is written here.
 *
 * gscales.c's two shared objects are declared here rather than by including
 * core/ti_hw/gscales.h: this module then needs no TiEmu header path, and the
 * host test (tests/test_t89_lcd.c) can link it without a TiEmu checkout.  The
 * declarations are gscales.h's own, for lcd_planebufs[3] and ngc.
 */
#include <stdint.h>

extern uint8_t *lcd_planebufs[];
extern int ngc;

#include "t89_lcd.h"
#include "t89_lcd_gray.h"

void t89_lcd_render(uint8_t *buffer)
{
    const uint8_t *planes[T89_LCD_MAX_PLANES];
    const int8_t *seq;
    unsigned current, i;

    if (!buffer) return;

    /* gscales.c only ever sets ngc to 1, 3, 7 or 8 (process_address); the app
     * seeds it to 1 after a reset, like gui/calc/calc.c did. */
    current = (ngc > 0) ? (unsigned)ngc : 1u;
    seq = t89_lcd_gray_seq(current);
    if (!seq) seq = t89_lcd_gray_seq(1u); /* unknown ngc: the model's b&w */

    for (i = 0u; i < T89_LCD_MAX_PLANES; i++) planes[i] = lcd_planebufs[i];
    t89_lcd_gray_compose(buffer, T89_LCD_ROW_BYTES, planes, seq,
                         T89_LCD_WIDTH, T89_LCD_HEIGHT);
}

const char *t89_lcd_mode(void)
{
    return t89_lcd_gray_mode((ngc > 0) ? (unsigned)ngc : 1u);
}
