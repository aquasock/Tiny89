/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Tiny89 grayscale LCD image: the calculator's LCD as a one-byte-per-pixel
 * image with four levels, for the new Tiny89 video unit to scan out of the
 * top 128 KiB of SDRAM (two buffers, buffer b at 0x41fe0000 + 0x10000*b,
 * pixel (x,y) at byte offset 256*y + x, 0 = paper, 3 = ink).  That unit scales
 * the picture 7x, centred at (80,10) in 1280x720, and takes its four colours
 * from PALETTE(0..3).
 *
 * The app owns the buffer pointer and the SDRAM; this module writes only
 * pixels (the first T89_LCD_WIDTH bytes of each row), so the bytes a row
 * carries beyond the picture stay whatever the app left there.
 *
 * The levels are not invented here: they are TiEmu's plane-exposure
 * composition, ported from the GTK frontend Tiny89 dropped -- see
 * t89_lcd.c and t89_lcd_gray.c for the exact TiEmu files and symbols.
 */
#ifndef T89_LCD_H
#define T89_LCD_H

#include <stdint.h>

#define T89_LCD_WIDTH 160u
#define T89_LCD_HEIGHT 100u
#define T89_LCD_ROW_BYTES 256u   /* SDRAM row stride; the first 160 bytes carry pixels */
#define T89_LCD_LEVELS 4

/* Writes T89_LCD_ROW_BYTES*T89_LCD_HEIGHT bytes: 100 rows of 160 levels, the
 * rest of each row untouched.  Reads TiEmu's gray-plane state (lcd_planebufs[]
 * and ngc), which the app seeds after a reset (see main.c). */
void t89_lcd_render(uint8_t *buffer);

/* "gray" when the plane model is in use, else "1bit". */
const char *t89_lcd_mode(void);

#endif
