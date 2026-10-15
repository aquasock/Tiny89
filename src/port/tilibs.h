/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef TINY89_PORT_TILIBS_H
#define TINY89_PORT_TILIBS_H

/* The AE350 runtime has no calculator-link backend. TiEmu keeps this type in
 * its public hardware state, so retain only its scalar configuration shape. */
typedef struct {
    int cable_delay;
    int cable_timeout;
    int cable_port;
    int cable_model;
    int calc_model;
} DeviceOptions;

typedef struct Tiny89CableHandle CableHandle;
typedef struct Tiny89CalcHandle CalcHandle;

#endif
