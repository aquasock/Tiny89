/* SPDX-License-Identifier: MIT */
/*
 * The keyboard TinyTang sends to the core, mapped onto the calculator's keypad.
 *
 * TinyTang pairs a Bluetooth keyboard and, since this cycle, forwards its HID
 * boot reports to a core as an input frame (type 0x17, seven bytes: the
 * modifier byte and six usage codes).  Tiny89's fabric parses those frames in
 * desk_link.sv and holds the newest report in two registers: kbd_lo is
 * {usage3, usage2, usage1, modifiers} and kbd_hi is {sequence, usage6, usage5,
 * usage4}.  A reader watches the sequence byte, and two reads give it a whole
 * report.
 *
 * The report is state, not events: a key held down sends one report and then
 * nothing, so a held key is one press until a new report says otherwise, and
 * releases are found by comparing with the last report, exactly as the pad
 * mapping did.
 *
 * Usages are HID keyboard page (0x07) codes, the ones a boot keyboard sends,
 * recorded in core-reference.md as KEY-001.  The mapping is this project's
 * choice and lives in two tables in t89_key.c: one for the keys as they are,
 * one for the keys Shift turns them into.  A key neither names reaches the
 * calculator as nothing, and Shift with a key the second table does not name
 * keeps that key's ordinary meaning.
 */
#ifndef T89_KEY_H
#define T89_KEY_H

#include <stdint.h>

#include "t89_pad.h"   /* t89_key_event_t, shared with the pad mapping */

#define T89_KBD_KEYS 6

typedef struct {
    uint8_t mods;               /* HID modifier bits */
    uint8_t keys[T89_KBD_KEYS]; /* HID usage codes, 0 = nothing */
} t89_kbd_t;

/* Split the two registers into a report and its sequence byte. */
void t89_kbd_decode(uint32_t lo, uint32_t hi, t89_kbd_t *out, uint8_t *seq);

/* The TiKey a usage code means, or -1 for one this project does not name. */
int t89_kbd_key(uint8_t usage);

/* The same usage held with Shift, which is a bit in the report's modifier byte
 * rather than a usage of its own: the calculator's plus, multiply, power and
 * negate, and the calculator's own F1 to F8.  -1 for a usage Shift does not
 * name, which callers read as "keep the unshifted meaning". */
int t89_kbd_shift_key(uint8_t usage);

/* The keyboard as the calculator sees it: the set of keys the report implies,
 * written to the emulator as the difference from the report the module last
 * applied, which it keeps itself.  At most max_events events, or -1 if the
 * difference does not fit, leaving the state unchanged (the callers pass 16).
 * Holding a key is one press and one release, the rule the pad mapping
 * followed. */
int t89_kbd_update(const t89_kbd_t *kbd, t89_key_event_t *events,
                   int max_events);

/* Forget what was held, so the next update presses what the report says. */
void t89_kbd_reset(void);

#endif
