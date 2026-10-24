/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef TINY89_PROTO_H
#define TINY89_PROTO_H

#include <stdint.h>

/* Send one variable to a calculator that is in Receive mode.  `name` is at
 * most 8 characters, `type` is a TI variable type (0x05 program, 0x24 flash
 * application), and `size` is the payload's length in bytes.  Returns non-zero
 * when the calculator acknowledged the whole exchange. */
int t89_proto_send_var(uint8_t type, const char *name, const uint8_t *data,
                       unsigned size);

#endif
