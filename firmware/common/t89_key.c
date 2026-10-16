/* SPDX-License-Identifier: MIT */
/*
 * The keyboard's report and the calculator's keypad.
 *
 * The register split and the report's shape come from this repository's
 * fabric: desk_link.sv parses TinyTang's input frame 0x17, whose payload is a
 * keyboard's HID boot report -- the modifier byte and six usage codes, which
 * is what TinyTang forwards from the Bluetooth keyboard it pairs
 * (ports/bl616/tang_osd_desk.c and tang_fpga_link.h in TinyTang).  TinyTang
 * sends a frame only when the report changes, so a report is state.
 *
 * The usage codes are HID keyboard page 0x07, HID Usage Tables for USB 1.4
 * section 10 (core-reference.md, KEY-001).  The two tables below are this
 * project's mapping, not a standard: the first names the arithmetic, digit,
 * letter, editing and calculator-only keys, the second the keys a PC keyboard
 * reaches with Shift held.  The standard's own names are why these four are
 * where they are -- 0x2E is "= and +" and 0x25 is "8 and *" -- so Shift with
 * the equals key is the calculator's plus and Shift with 8 is its multiply.
 * A key neither table names reaches the calculator as nothing, and the
 * modifier byte does nothing beyond choosing between the two tables; 2nd, the
 * diamond, ALPHA and MODE still sit on Tab and F1 to F3, so no key needs a
 * modifier held for the calculator to see it.
 */
#include <stddef.h>

#include "keydefs.h"
#include "t89_key.h"

typedef struct {
    uint8_t usage;
    int key;
} t89_bind_t;

static const t89_bind_t t89_binds[] = {
    /* the four arrows, by their HID usage names: 0x52 Up, 0x51 Down,
     * 0x50 Left, 0x4F Right.  Left and right were the wrong way round until
     * 2026-10-16 -- the table named 0x4F left and 0x50 right, which the host
     * test did not catch because it checked only up and down. */
    { 0x52, TIKEY_UP }, { 0x51, TIKEY_DOWN },
    { 0x50, TIKEY_LEFT }, { 0x4F, TIKEY_RIGHT },
    /* editing */
    { 0x28, TIKEY_ENTER1 }, { 0x29, TIKEY_ESCAPE }, { 0x2A, TIKEY_BACKSPACE },
    /* the calculator's own modifiers and menus, on the top row */
    { 0x2B, TIKEY_2ND }, { 0x3A, TIKEY_DIAMOND }, { 0x3B, TIKEY_ALPHA },
    { 0x3C, TIKEY_MODE }, { 0x3D, TIKEY_CATALOG },
    /* digits, along the number row */
    { 0x1E, TIKEY_1 }, { 0x1F, TIKEY_2 }, { 0x20, TIKEY_3 }, { 0x21, TIKEY_4 },
    { 0x22, TIKEY_5 }, { 0x23, TIKEY_6 }, { 0x24, TIKEY_7 }, { 0x25, TIKEY_8 },
    { 0x26, TIKEY_9 }, { 0x27, TIKEY_0 },
    /* letters */
    { 0x04, TIKEY_A }, { 0x05, TIKEY_B }, { 0x06, TIKEY_C }, { 0x07, TIKEY_D },
    { 0x08, TIKEY_E }, { 0x09, TIKEY_F }, { 0x0A, TIKEY_G }, { 0x0B, TIKEY_H },
    { 0x0C, TIKEY_I }, { 0x0D, TIKEY_J }, { 0x0E, TIKEY_K }, { 0x0F, TIKEY_L },
    { 0x10, TIKEY_M }, { 0x11, TIKEY_N }, { 0x12, TIKEY_O }, { 0x13, TIKEY_P },
    { 0x14, TIKEY_Q }, { 0x15, TIKEY_R }, { 0x16, TIKEY_S }, { 0x17, TIKEY_T },
    { 0x18, TIKEY_U }, { 0x19, TIKEY_V }, { 0x1A, TIKEY_W }, { 0x1B, TIKEY_X },
    { 0x1C, TIKEY_Y }, { 0x1D, TIKEY_Z },
    /* arithmetic and punctuation, including the keypad */
    { 0x2D, TIKEY_MINUS }, { 0x2E, TIKEY_EQUALS }, { 0x2F, TIKEY_PALEFT },
    { 0x30, TIKEY_PARIGHT }, { 0x33, TIKEY_COMMA }, { 0x34, TIKEY_PIPE },
    { 0x36, TIKEY_COMMA }, { 0x37, TIKEY_PERIOD }, { 0x38, TIKEY_DIVIDE },
    { 0x54, TIKEY_DIVIDE }, { 0x55, TIKEY_MULTIPLY }, { 0x56, TIKEY_MINUS },
    { 0x57, TIKEY_PLUS }, { 0x58, TIKEY_ENTER2 }, { 0x63, TIKEY_PERIOD },
    /* the calculator's own keys, on the PC keys that have no other job here:
     * the spacebar, the editing cluster, Caps Lock and the F row.  The
     * editing cluster reads as the names suggest -- Home is home, Delete
     * clears, End ends the operation -- and the F row carries the functions
     * in the order the calculator's own keypad groups them. */
    { 0x2C, TIKEY_SPACE }, { 0x39, TIKEY_HAND }, { 0x49, TIKEY_APPS },
    { 0x4A, TIKEY_HOME }, { 0x4B, TIKEY_STORE }, { 0x4C, TIKEY_CLEAR },
    { 0x4D, TIKEY_ON }, { 0x4E, TIKEY_EE },
    { 0x3F, TIKEY_SIN }, { 0x40, TIKEY_COS }, { 0x41, TIKEY_TAN },
    { 0x42, TIKEY_LN }, { 0x43, TIKEY_NU }, { 0x44, TIKEY_THETA }
};

/* Held with Shift.  Shift has no key of its own in a report -- it is a bit in
 * the modifier byte -- so these are the meanings the modifier gives the keys
 * above rather than a second keypad.  TIKEY_SHIFT is deliberately not here:
 * the shift bit is what chooses between the tables, and forwarding it as a
 * key as well would press the calculator's shift on every shifted key. */
static const t89_bind_t t89_shift_binds[] = {
    { 0x2E, TIKEY_PLUS }, { 0x25, TIKEY_MULTIPLY },
    { 0x23, TIKEY_POWER }, { 0x2D, TIKEY_NEGATE },
    /* the calculator's own F1 to F8, on the PC's F1 to F8 with Shift, which
     * leaves F1 to F4 unshifted free for the diamond, ALPHA, MODE and
     * CATALOG that were already there */
    { 0x3A, TIKEY_F1 }, { 0x3B, TIKEY_F2 }, { 0x3C, TIKEY_F3 },
    { 0x3D, TIKEY_F4 }, { 0x3E, TIKEY_F5 }, { 0x3F, TIKEY_F6 },
    { 0x40, TIKEY_F7 }, { 0x41, TIKEY_F8 }
};

#define T89_BIND_COUNT (sizeof(t89_binds) / sizeof(t89_binds[0]))
#define T89_SHIFT_BIND_COUNT \
    (sizeof(t89_shift_binds) / sizeof(t89_shift_binds[0]))

/* The shift bits of the boot report's modifier byte: usages 0xE1 LeftShift
 * and 0xE5 RightShift, whose low bits are 0x02 and 0x20. */
#define T89_MOD_SHIFT 0x22u

/* The keys the module last applied, so a report is a difference and a held
 * key is one press. */
static unsigned char t89_applied[NB_MAX_KEYS];

int t89_kbd_key(uint8_t usage)
{
    size_t i;

    if (usage == 0) return -1;
    for (i = 0; i < T89_BIND_COUNT; i++) {
        if (t89_binds[i].usage == usage) return t89_binds[i].key;
    }
    return -1;
}

int t89_kbd_shift_key(uint8_t usage)
{
    size_t i;

    if (usage == 0) return -1;
    for (i = 0; i < T89_SHIFT_BIND_COUNT; i++) {
        if (t89_shift_binds[i].usage == usage) return t89_shift_binds[i].key;
    }
    return -1;
}

void t89_kbd_decode(uint32_t lo, uint32_t hi, t89_kbd_t *out, uint8_t *seq)
{
    if (!out) return;

    /* kbd_lo is {usage3, usage2, usage1, modifiers} and kbd_hi is
     * {sequence, usage6, usage5, usage4}, both red from the fabric. */
    out->mods = (uint8_t)(lo & 0xffu);
    out->keys[0] = (uint8_t)((lo >> 8) & 0xffu);
    out->keys[1] = (uint8_t)((lo >> 16) & 0xffu);
    out->keys[2] = (uint8_t)((lo >> 24) & 0xffu);
    out->keys[3] = (uint8_t)(hi & 0xffu);
    out->keys[4] = (uint8_t)((hi >> 8) & 0xffu);
    out->keys[5] = (uint8_t)((hi >> 16) & 0xffu);
    if (seq) *seq = (uint8_t)((hi >> 24) & 0xffu);
}

void t89_kbd_reset(void)
{
    int i;

    for (i = 0; i < NB_MAX_KEYS; i++) t89_applied[i] = 0;
}

static void implied(const t89_kbd_t *kbd, unsigned char *set)
{
    const int shift = (kbd->mods & T89_MOD_SHIFT) != 0;
    int i;

    for (i = 0; i < NB_MAX_KEYS; i++) set[i] = 0;
    for (i = 0; i < T89_KBD_KEYS; i++) {
        int key = shift ? t89_kbd_shift_key(kbd->keys[i]) : -1;

        /* A key the shift table does not name keeps its unshifted meaning,
         * so Shift with an arrow or a letter is still that same key. */
        if (key < 0) key = t89_kbd_key(kbd->keys[i]);
        if (key >= 0 && key < NB_MAX_KEYS) set[key] = 1;
    }
}

int t89_kbd_update(const t89_kbd_t *kbd, t89_key_event_t *events,
                   int max_events)
{
    unsigned char want[NB_MAX_KEYS];
    int releases = 0, presses = 0, n = 0, i;

    if (!kbd || max_events < 0) return -1;

    implied(kbd, want);

    for (i = 0; i < NB_MAX_KEYS; i++) {
        if (t89_applied[i] && !want[i]) releases++;
        else if (!t89_applied[i] && want[i]) presses++;
    }
    if (releases + presses > max_events) return -1;
    if (releases + presses > 0 && !events) return -1;

    for (i = 0; i < NB_MAX_KEYS; i++) {
        if (t89_applied[i] && !want[i]) {
            events[n].key = i;
            events[n].active = 0;
            n++;
        }
    }
    for (i = 0; i < NB_MAX_KEYS; i++) {
        if (!t89_applied[i] && want[i]) {
            events[n].key = i;
            events[n].active = 1;
            n++;
        }
    }
    for (i = 0; i < NB_MAX_KEYS; i++) t89_applied[i] = want[i];

    return n;
}
