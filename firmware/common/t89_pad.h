/* SPDX-License-Identifier: MIT */
#ifndef T89_PAD_H
#define T89_PAD_H
#include <stdint.h>
#include "keydefs.h"

/* The fifteen active-low buttons of a Wii Classic Controller, as the word
 * TinyNES's pad_test decodes: ~((byte5 << 8) | byte4), byte 4 in bits 7:0. */
enum {
    T89_PAD_R      = 1u << 1,
    T89_PAD_START  = 1u << 2,
    T89_PAD_HOME   = 1u << 3,
    T89_PAD_SELECT = 1u << 4,
    T89_PAD_L      = 1u << 5,
    T89_PAD_DOWN   = 1u << 6,
    T89_PAD_RIGHT  = 1u << 7,
    T89_PAD_UP     = 1u << 8,
    T89_PAD_LEFT   = 1u << 9,
    T89_PAD_ZR     = 1u << 10,
    T89_PAD_X      = 1u << 11,
    T89_PAD_A      = 1u << 12,
    T89_PAD_Y      = 1u << 13,
    T89_PAD_B      = 1u << 14,
    T89_PAD_ZL     = 1u << 15
};

typedef struct {
    uint16_t buttons;   /* active high: 1 = pressed */
    uint8_t left_x;     /* left stick, 6 bits, 32 = centre */
    uint8_t left_y;
} t89_pad_t;

typedef struct {
    int key;            /* a TiKey */
    int active;         /* 1 press, 0 release */
} t89_key_event_t;

#define T89_LAYER_COUNT 3

typedef struct {
    int layer;                          /* 0 nav, 1 digits, 2 math */
    int applied_count;                  /* keys this module last applied */
    unsigned char applied[NB_MAX_KEYS]; /* indexed by TiKey; module-private */
} t89_keymap_t;

/* Decode one sample of controller n's registers: DATA(n) in lo and DATA2(n)
 * bits 15:0 in hi.  This is raw -- it decodes whatever the words hold, and a
 * controller that is absent leaves words that decode to a stick held at an
 * extreme.  With no controller, call t89_pad_none() instead of decoding. */
void t89_pad_decode(uint32_t lo, uint32_t hi, t89_pad_t *out);

/* The pad to apply when no controller is present or it did not identify as a
 * Classic Controller: nothing pressed and both stick axes centred.  A zeroed
 * t89_pad_t is not that state -- axis 0 is the stick's minimum, which reads as
 * up-left. */
void t89_pad_none(t89_pad_t *out);

void t89_keymap_reset(t89_keymap_t *map);
int  t89_keymap_update(t89_keymap_t *map, const t89_pad_t *pad, t89_key_event_t *events, int max_events);
const char *t89_layer_name(int layer);
int  t89_layer_key(int layer, uint16_t button);
int  t89_layer_direction(int layer, int direction);
#endif
