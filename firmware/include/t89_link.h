/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef TINY89_LINK_H
#define TINY89_LINK_H

#include <stdint.h>

/* Point the emulator's link-port hooks at this driver.  Call once, after
 * hw_init()/hw_reset(): the calculator's Link app then drives real wires. */
void t89_link_install(void);

/* The calculator's next transmitted byte, or -1. */
int t89_link_read(void);

/* Queue a byte for the calculator; false when its queue is full. */
int t89_link_write(uint8_t value);

/* True when the calculator has sent something not yet read. */
int t89_link_pending(void);

/* One console line: wire writes and reads, bytes out and in. */
void t89_link_report(const char *when);

#endif
