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
 * The usage codes are HID keyboard page 0x07.  The table below is this
 * project's mapping, not a standard: it names the arithmetic, digit, letter
 * and editing keys the calculator has, and anything it does not name reaches
 * the calculator as nothing.  The modifiers are deliberately unmapped; 2nd,
 * the diamond, ALPHA and MODE sit on Tab and F1 to F3 instead, so that no key
 * needs a modifier held for the calculator to see it.
 */
#include <stddef.h>

#include "keydefs.h"
#include "t89_key.h"

typedef struct {
    uint8_t usage;
    int key;
} t89_bind_t;

static const t89_bind_t t89_binds[] = {
    /* the four arrows */
    { 0x52, TIKEY_UP }, { 0x51, TIKEY_DOWN },
    { 0x4F, TIKEY_LEFT }, { 0x50, TIKEY_RIGHT },
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
    { 0x57, TIKEY_PLUS }, { 0x58, TIKEY_ENTER1 }, { 0x63, TIKEY_PERIOD }
};

#define T89_BIND_COUNT (sizeof(t89_binds) / sizeof(t89_binds[0]))

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
    int i;

    for (i = 0; i < NB_MAX_KEYS; i++) set[i] = 0;
    for (i = 0; i < T89_KBD_KEYS; i++) {
        int key = t89_kbd_key(kbd->keys[i]);

        if (key >= 0 && key < NB_MAX_KEYS) set[key] = 1;
    }
    /* The modifier byte is unmapped by design, so nothing else is implied. */
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
