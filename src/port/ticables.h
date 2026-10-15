/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef TINY89_PORT_TICABLES_H
#define TINY89_PORT_TICABLES_H
typedef struct Tiny89CableHandle CableHandle;
static inline int ticables_cable_set_d0(CableHandle *handle, int value)
{ (void)handle; (void)value; return 0; }
static inline int ticables_cable_set_d1(CableHandle *handle, int value)
{ (void)handle; (void)value; return 0; }
static inline int ticables_cable_get_d0(CableHandle *handle)
{ (void)handle; return 0; }
static inline int ticables_cable_get_d1(CableHandle *handle)
{ (void)handle; return 0; }
#endif
