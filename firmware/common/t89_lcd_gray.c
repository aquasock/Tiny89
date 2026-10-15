/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * TiEmu's plane-exposure composition, ported to Tiny89 and kept pure.
 *
 * Ported from third_party/tiemu/tiemu/trunk/src/gui/calc/screen.c at the
 * pinned commit:
 *   const int gp_seq[9][8]        -- which planes shade a pixel, per ngc
 *   void compute_convtable(void)  -- byte -> eight bit values, MSB first
 *   int hid_update_lcd(void)      -- the loop that sums the sequence into
 *                                    lcd_bytmap[] and shades from
 *                                    grayscales[] via compute_grayscale()
 * and from core/ti_hw/gscales.c: lcd_planebufs[3], ngc, lcd_planes[],
 * lcd_changed (process_address()), which core/ti_hw/m68k.c drives from the
 * run loop with lcd_hook_hw2(0) for HW2/HW3.
 *
 * screen.c summed a convtab[] entry per set bit across the gp_seq exposures,
 * which is exactly "how many of the exposures the pixel is set in".  That sum
 * is computed here directly; the level it produces is
 *   sum over seq of plane[seq[i]]'s bit at (x,y)
 * straight for ngc = 1 (0..1) and ngc = 3 (0..3), and quantized onto the four
 * levels Tiny89's video unit reads for ngc = 7 (0..6) and ngc = 8 (0..7).
 * See t89_lcd_gray.h for the model's own limits.
 */
#include <stddef.h>

#include "t89_lcd_gray.h"

/*
 * gp_seq, copied verbatim from screen.c.  Index is ngc, entries are plane
 * indices into lcd_planebufs[], -1 terminates; the unused rows are {-1}.
 */
static const int8_t gp_seq[9][8] = {
    { -1 },                     /* unused */
    { 0, -1 },                  /* b&w      (1 plane)  */
    { -1 },                     /* unused */
    { 0, 0, 1, -1 },            /* 4 colors (2 planes) */
    { -1 },                     /* unused */
    { -1 },                     /* unused */
    { -1 },                     /* unused */
    { 2, 0, 1, 0, 1, 0, -1 },   /* 7 colors (3 planes) */
    { 1, 0, 2, 0, 0, 1, 0, -1 } /* 8 colors (3 planes) */
};

const int8_t *t89_lcd_gray_seq(unsigned ngc)
{
    const int8_t *seq;

    if (ngc > 8u) return 0;
    seq = gp_seq[ngc];
    if (seq[0] < 0) return 0;
    return seq;
}

const char *t89_lcd_gray_mode(unsigned ngc)
{
    return (ngc > 1u) ? "gray" : "1bit";
}

void t89_lcd_gray_compose(uint8_t *out, unsigned out_stride,
                          const uint8_t *const planes[T89_LCD_MAX_PLANES],
                          const int8_t *seq,
                          unsigned width, unsigned height)
{
    unsigned x, y, exposures, i;

    if (!out || !seq || width == 0u || height == 0u) return;

    /* The number of exposures the model composes = the model's top level. */
    for (exposures = 0u; exposures < T89_LCD_MAX_EXPOSURES
         && seq[exposures] >= 0; exposures++) {
        ;
    }
    if (exposures == 0u || exposures >= T89_LCD_MAX_EXPOSURES) return;

    for (y = 0u; y < height; y++) {
        for (x = 0u; x < width; x++) {
            unsigned raw = 0u;
            uint8_t mask = (uint8_t)(0x80u >> (x & 7u));
            unsigned column = y * T89_LCD_PLANE_ROW_BYTES + (x >> 3);

            for (i = 0u; i < exposures; i++) {
                unsigned plane = (unsigned)seq[i];
                const uint8_t *bits;

                if (plane >= T89_LCD_MAX_PLANES) continue; /* not a plane */
                bits = planes[plane];
                if (!bits) continue;             /* not filled in yet: paper */
                if (bits[column] & mask) raw++;
            }
            /* Spread the model's 0..exposures onto 0..T89_LCD_GRAY_LEVELS-1,
             * keeping both ends exact (0 paper, exposures ink). */
            out[(size_t)y * out_stride + x] = (uint8_t)
                ((raw * (T89_LCD_GRAY_LEVELS - 1u) + exposures / 2u) / exposures);
        }
    }
}
