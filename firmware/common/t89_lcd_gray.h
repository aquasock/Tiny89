/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * The pure half of the Tiny89 grayscale LCD: TiEmu's plane-exposure
 * composition, with the plane state passed in as arguments so it can be
 * exercised on the host without TiEmu's globals (tests/test_t89_lcd.c).
 *
 * Source of the port, third_party/tiemu at the pinned commit:
 *   gui/calc/screen.c : gp_seq[9][8], compute_convtable(), compute_grayscale(),
 *                       max_plane, hid_update_lcd() (the composition loop)
 *   core/ti_hw/gscales.c : lcd_planebufs[3], lcd_planes[5], ngc, lcd_changed,
 *                          process_address()
 *
 * In screen.c the composition is: memset(lcd_bytmap, 0, ...) then, for every
 * plane p in the sequence gp_seq[ngc] (stopping at -1), add the plane's bits
 * into lcd_bytmap with convtab (one byte per pixel, MSB first); the resulting
 * value is the sum over the exposures, and grayscales[] then shades it.  That
 * sum is what this module computes: the level is how many times the pixel is
 * exposed across the plane sequence, i.e. how many planes are in use (ngc) and
 * which planes the pixel is set in.
 *
 * TiEmu's ngc is 1, 3, 7 or 8 (gscales.c, process_address(): 1 plane -> 1,
 * 2 planes -> 3, 3 planes -> 7 or 8 by exposure ratio), matching gp_seq's
 * used rows; rows 0, 2, 4, 5, 6 are marked unused in screen.c.  ngc = 3
 * (gp_seq {0,0,1}) gives exactly four levels 0..3 and is the only shape whose
 * four levels come straight out of the model; ngc = 1 gives two, and ngc = 7
 * and 8 give the model's 7/8 levels, which this module quantizes onto four.
 */
#ifndef T89_LCD_GRAY_H
#define T89_LCD_GRAY_H

#include <stdint.h>

/* TiEmu's LCD memory: 240 bit-columns by 128 rows, MSB first, 30 bytes a row
 * (core/ti68k_def.h: LCDMEM_W 240, LCDMEM_H 128).  The calculator's 160x100
 * picture is its top-left corner, as in the app's 1-bit renderer. */
#define T89_LCD_PLANE_COLS 240u
#define T89_LCD_PLANE_ROW_BYTES (T89_LCD_PLANE_COLS / 8u)

#define T89_LCD_MAX_PLANES 3u
#define T89_LCD_GRAY_LEVELS 4u
/* gp_seq rows are 8 entries wide and -1 terminated (screen.c), so a sequence
 * is at most 8 exposures. */
#define T89_LCD_MAX_EXPOSURES 8u

/* gp_seq[ngc] as TiEmu's screen.c declares it: -1 terminated.  Returns NULL
 * for an ngc TiEmu has no sequence for (0, 2, 4, 5, 6). */
const int8_t *t89_lcd_gray_seq(unsigned ngc);

/* "gray" when more than one plane is composed (ngc > 1), else "1bit".  The
 * two strings are static, so the result is stable and comparable by pointer. */
const char *t89_lcd_gray_mode(unsigned ngc);

/* Compose the sequence's planes into out: height rows of width bytes at
 * out_stride, each byte 0..T89_LCD_GRAY_LEVELS-1 (0 paper).  Bytes at
 * offset >= width in a row are not touched.  A NULL plane, or a plane index
 * outside planes[], counts as an all-paper plane rather than crashing: the
 * callers run before gscales.c has filled every buffer in. */
void t89_lcd_gray_compose(uint8_t *out, unsigned out_stride,
                          const uint8_t *const planes[T89_LCD_MAX_PLANES],
                          const int8_t *seq,
                          unsigned width, unsigned height);

#endif
