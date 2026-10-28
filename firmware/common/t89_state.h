/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef T89_STATE_H
#define T89_STATE_H

#include <stdint.h>

/*
 * The save state's layout: one definition, used by the board's program and by
 * the browser app, so a state written in one is read by the other.
 *
 * Why a state and not a RAM image.  A TI-89 operating system rebuilds its RAM
 * at every cold boot: given a RAM image captured from a running machine, a
 * boot discards the user's variables and rewrites about 24 KB before it
 * reaches the desktop (measured in core-log entry 22 -- a `pacman` variable
 * present in the image was gone after the boot).  So a flash-and-RAM pair
 * cannot carry anything; only a machine that is resumed rather than booted
 * can.  The state holds the whole machine, and restoring it ends with the
 * calculator continuing at the instruction it was at.
 *
 * The bytes, in this order:
 *
 *   state_header_t   the sizes, so a reader from another build can refuse
 *   struct regs      the UAE register file, exactly as TiEmu keeps it
 *   io, io2, io3     the three port banks, at their own sizes
 *   RAM              at its own size
 *   state_extras_t   the hardware fields and the live program counter
 *
 * No CRC: the transport carries one (Tiny89's save protocol has a CRC in its
 * header), and the blob itself is judged by the header's magic and sizes.
 */
#define T89_STATE_MAGIC 0x53393854u     /* "T89S" */
#define T89_STATE_VERSION 2u

typedef struct {
    uint32_t magic;
    uint32_t version;
    uint32_t regs_size;
    uint32_t io_size;
    uint32_t io2_size;
    uint32_t io3_size;
    uint32_t ram_size;
    uint32_t extras_size;
} state_header_t;

/* Ti68kHardware's state fields, the ones ti68k_state_load copies deliberately:
 * the pointers and sizes around them are layout and are rebuilt by hw_init.
 * `pc` is the live program counter: regs.pc is not it, because UAE keeps pc at
 * the last m68k_setpc and lets pc_p advance through the instruction, so the
 * true address is m68k_getpc() and resuming from regs.pc replays instructions
 * with their side effects. */
typedef struct {
    uint32_t lcd_adr;
    int32_t contrast;
    int32_t on_off;
    int32_t protect;
    uint32_t archive_limit;
    int32_t ram_exec[64];
    uint8_t timer_value;
    uint8_t timer_init;
    uint8_t rtc_value;
    uint8_t pad;
    uint32_t pc;
} state_extras_t;

#endif
