/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef TINY89_PORT_TICABLES_H
#define TINY89_PORT_TICABLES_H
typedef struct Tiny89CableHandle CableHandle;

/* The wires, implemented in firmware/common/t89_link.c: ports.c calls the
 * setters when the calculator drives the pair and the getters when it reads
 * them, so the same four calls are both directions of one open-collector
 * bus. */
int ticables_cable_set_d0(CableHandle *handle, int value);
int ticables_cable_set_d1(CableHandle *handle, int value);
int ticables_cable_get_d0(CableHandle *handle);
int ticables_cable_get_d1(CableHandle *handle);
#endif
