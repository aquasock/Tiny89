/* SPDX-License-Identifier: MIT */
/*
 * Host tests for firmware/common/t89_pad.c -- the Wii Classic Controller to
 * TI-89 keypad mapping.  Plain C, no framework: every check is one named
 * comparison, mismatches print "FAIL <name>" and are counted.
 *
 * The expectations below restate the mapping on purpose: the table in
 * t89_pad.c is checked against the intended mapping, not against itself.
 */
#include <stdio.h>
#include <string.h>

#include "t89_pad.h"

static int g_checks;
static int g_fails;
static char g_msg[192];

static void check(int cond, const char *name)
{
    g_checks++;
    if (!cond) {
        g_fails++;
        printf("FAIL %s\n", name);
    }
}

#define CHECK(cond, ...)                                                      \
    do {                                                                      \
        snprintf(g_msg, sizeof g_msg, __VA_ARGS__);                            \
        check((cond), g_msg);                                                  \
    } while (0)

#define NEV 16
static t89_key_event_t g_ev[NEV];
static int g_ev_n;

static t89_pad_t mkpad(uint16_t buttons, uint8_t x, uint8_t y)
{
    t89_pad_t p;

    p.buttons = buttons;
    p.left_x = x;
    p.left_y = y;
    return p;
}

/* A pad with the stick centred: only the buttons matter. */
static t89_pad_t pad_buttons(uint16_t buttons)
{
    return mkpad(buttons, 32, 32);
}

static int upd_ev(t89_keymap_t *m, t89_pad_t p, int max_events)
{
    g_ev_n = t89_keymap_update(m, &p, g_ev, max_events);
    return g_ev_n;
}

static int upd(t89_keymap_t *m, t89_pad_t p)
{
    return upd_ev(m, p, NEV);
}

#define CHECK_EV(i, k, a, ...)                                                \
    do {                                                                      \
        snprintf(g_msg, sizeof g_msg, __VA_ARGS__);                            \
        check((i) < g_ev_n && g_ev[(i)].key == (k) && g_ev[(i)].active == (a),  \
              g_msg);                                                         \
    } while (0)

/* All fifteen button bits, in the order the pad_test word lists them. */
static const uint16_t k_buttons[15] = {
    T89_PAD_R,    T89_PAD_START, T89_PAD_HOME, T89_PAD_SELECT, T89_PAD_L,
    T89_PAD_DOWN, T89_PAD_RIGHT, T89_PAD_UP,   T89_PAD_LEFT,   T89_PAD_ZR,
    T89_PAD_X,    T89_PAD_A,     T89_PAD_Y,    T89_PAD_B,      T89_PAD_ZL
};
static const char *k_button_name[15] = {
    "R", "START", "HOME", "SELECT", "L", "DOWN", "RIGHT", "UP", "LEFT",
    "ZR", "X", "A", "Y", "B", "ZL"
};

static const int k_expect[3][15] = {
    /* nav */    { TIKEY_MODE,  TIKEY_HOME,  -1, TIKEY_APPS,    TIKEY_ALPHA,
                   TIKEY_DOWN, TIKEY_RIGHT, TIKEY_UP,   TIKEY_LEFT,  TIKEY_CATALOG,
                   TIKEY_2ND,  TIKEY_ENTER1, TIKEY_DIAMOND, TIKEY_ESCAPE, -1 },
    /* digits */ { TIKEY_7,     TIKEY_ENTER1, -1, TIKEY_NEGATE, TIKEY_9,
                   TIKEY_2,     TIKEY_6,     TIKEY_8,   TIKEY_4,     TIKEY_PERIOD,
                   TIKEY_1,     TIKEY_5,     TIKEY_3,   TIKEY_0,     -1 },
    /* math */   { TIKEY_EQUALS, TIKEY_ENTER1, -1, TIKEY_NEGATE, TIKEY_POWER,
                   TIKEY_DOWN,  TIKEY_RIGHT, TIKEY_UP,  TIKEY_LEFT,  TIKEY_PARIGHT,
                   TIKEY_MULTIPLY, TIKEY_PLUS, TIKEY_DIVIDE, TIKEY_MINUS, -1 }
};

static void test_decode(void)
{
    t89_pad_t p;

    /* A real report: byte4 = 0xEF clears bit 4 (SELECT), byte5 = 0xFF is
     * idle, and both axes sit at 0x20 = centre. */
    t89_pad_decode(0x2020u, 0xffefu, &p);
    CHECK(p.buttons == T89_PAD_SELECT, "decode: SELECT from active-low word");
    CHECK(p.left_x == 32, "decode: left_x 32 is centre");
    CHECK(p.left_y == 32, "decode: left_y 32 is centre");

    /* Its inverse: the same word with that bit back high is idle. */
    t89_pad_decode(0x2020u, 0xffffu, &p);
    CHECK(p.buttons == 0, "decode: inverse word presses nothing");

    /* Absent pad. */
    t89_pad_decode(0xffffffffu, 0xffffffffu, &p);
    CHECK(p.buttons == 0, "decode: absent pad presses nothing");

    /* Whole report low: bits 1..15 pressed, bit 0 unused. */
    t89_pad_decode(0x00000000u, 0x00000000u, &p);
    CHECK(p.buttons == 0xfffeu, "decode: all-low word presses bits 1..15");
    CHECK((p.buttons & 1u) == 0, "decode: bit 0 never pressed");

    /* Report bit 0 is unused and must not leak into the button word. */
    t89_pad_decode(0x00000000u, 0x00000001u, &p);
    CHECK(p.buttons == 0xfffeu, "decode: report bit 0 ignored");
    CHECK((p.buttons & 1u) == 0, "decode: report bit 0 masked off");

    /* Two buttons at once: byte5 = 0xFB clears bit 10 (ZR). */
    t89_pad_decode(0x00000000u, 0xfbefu, &p);
    CHECK(p.buttons == (uint16_t)(T89_PAD_ZR | T89_PAD_SELECT),
          "decode: two buttons in one word");

    /* 6-bit axes: bits 7:6 of each byte are not part of the value. */
    t89_pad_decode(0x00c0u, 0xffffffffu, &p);
    CHECK(p.left_x == 0 && p.left_y == 0, "decode: axis bits 7:6 masked off");
    t89_pad_decode(0x3fffu, 0xffffffffu, &p);
    CHECK(p.left_x == 63 && p.left_y == 63, "decode: 6-bit axes saturate at 63");
    t89_pad_decode(0x0c3fu, 0xffffffffu, &p);
    CHECK(p.left_x == 63, "decode: left_x is byte0 bits 5:0");
    CHECK(p.left_y == 12, "decode: left_y is byte1 bits 5:0 (lo >> 8)");
}

static void test_layer_name(void)
{
    CHECK(strcmp(t89_layer_name(0), "nav") == 0, "layer_name: 0 is nav");
    CHECK(strcmp(t89_layer_name(1), "digits") == 0, "layer_name: 1 is digits");
    CHECK(strcmp(t89_layer_name(2), "math") == 0, "layer_name: 2 is math");
    CHECK(t89_layer_name(-1) == NULL, "layer_name: -1 is NULL");
    CHECK(t89_layer_name(3) == NULL, "layer_name: 3 is NULL");
    CHECK(t89_layer_name(99) == NULL, "layer_name: 99 is NULL");
}

static void test_layer_keys(void)
{
    int layer, i;

    for (layer = 0; layer < T89_LAYER_COUNT; layer++) {
        for (i = 0; i < 15; i++) {
            CHECK(t89_layer_key(layer, k_buttons[i]) == k_expect[layer][i],
                  "layer_key: %s in layer %d (%s)", k_button_name[i], layer,
                  t89_layer_name(layer));
        }
    }

    /* HOME belongs to the fabric in every layer. */
    CHECK(t89_layer_key(0, T89_PAD_HOME) == -1 &&
          t89_layer_key(1, T89_PAD_HOME) == -1 &&
          t89_layer_key(2, T89_PAD_HOME) == -1,
          "layer_key: HOME unmapped in every layer");

    /* ZL only cycles the layer. */
    CHECK(t89_layer_key(0, T89_PAD_ZL) == -1 &&
          t89_layer_key(1, T89_PAD_ZL) == -1 &&
          t89_layer_key(2, T89_PAD_ZL) == -1,
          "layer_key: ZL never emits a key");

    CHECK(t89_layer_key(-1, T89_PAD_A) == -1, "layer_key: negative layer is -1");
    CHECK(t89_layer_key(3, T89_PAD_A) == -1, "layer_key: layer 3 is -1");
    CHECK(t89_layer_key(0, 0) == -1, "layer_key: zero button is -1");
    CHECK(t89_layer_key(0, 1u) == -1, "layer_key: unused bit 0 is -1");
    CHECK(t89_layer_key(0, (uint16_t)(T89_PAD_A | T89_PAD_B)) == -1,
          "layer_key: multi-bit button is -1");
}

static void test_layer_directions(void)
{
    CHECK(t89_layer_direction(0, 0) == TIKEY_UP, "direction: nav 0 is UP");
    CHECK(t89_layer_direction(0, 1) == TIKEY_DOWN, "direction: nav 1 is DOWN");
    CHECK(t89_layer_direction(0, 2) == TIKEY_LEFT, "direction: nav 2 is LEFT");
    CHECK(t89_layer_direction(0, 3) == TIKEY_RIGHT, "direction: nav 3 is RIGHT");
    CHECK(t89_layer_direction(1, 0) == TIKEY_8, "direction: digits 0 is 8");
    CHECK(t89_layer_direction(1, 1) == TIKEY_2, "direction: digits 1 is 2");
    CHECK(t89_layer_direction(1, 2) == TIKEY_4, "direction: digits 2 is 4");
    CHECK(t89_layer_direction(1, 3) == TIKEY_6, "direction: digits 3 is 6");
    CHECK(t89_layer_direction(2, 0) == TIKEY_UP, "direction: math 0 is UP");
    CHECK(t89_layer_direction(2, 1) == TIKEY_DOWN, "direction: math 1 is DOWN");
    CHECK(t89_layer_direction(2, 2) == TIKEY_LEFT, "direction: math 2 is LEFT");
    CHECK(t89_layer_direction(2, 3) == TIKEY_RIGHT, "direction: math 3 is RIGHT");
    CHECK(t89_layer_direction(0, 4) == -1, "direction: 4 is out of range");
    CHECK(t89_layer_direction(0, -1) == -1, "direction: -1 is out of range");
    CHECK(t89_layer_direction(3, 0) == -1, "direction: layer 3 is -1");
}

static void test_press_and_release(void)
{
    t89_keymap_t m;
    int n;

    t89_keymap_reset(&m);
    CHECK(m.layer == 0 && m.applied_count == 0, "reset: layer 0, nothing applied");

    /* nav: A is TIKEY_ENTER1. */
    n = upd(&m, pad_buttons(T89_PAD_A));
    CHECK(n == 1, "update: one event for one new key");
    CHECK_EV(0, TIKEY_ENTER1, 1, "update: A presses ENTER1 in nav");
    CHECK(m.applied_count == 1, "update: applied_count follows the key");

    /* A key held across updates emits exactly one press. */
    n = upd(&m, pad_buttons(T89_PAD_A));
    CHECK(n == 0, "update: held key does not repeat");
    n = upd(&m, pad_buttons(T89_PAD_A));
    CHECK(n == 0, "update: still held, still silent");

    n = upd(&m, pad_buttons(0));
    CHECK(n == 1, "update: release emits one event");
    CHECK_EV(0, TIKEY_ENTER1, 0, "update: A releases ENTER1");
    CHECK(m.applied_count == 0, "update: applied_count back to zero");
    n = upd(&m, pad_buttons(0));
    CHECK(n == 0, "update: released key does not repeat");
}

static void test_home_silence(void)
{
    t89_keymap_t m;
    int n;

    t89_keymap_reset(&m);
    n = upd(&m, pad_buttons(T89_PAD_HOME));
    CHECK(n == 0, "home: presses no key");
    CHECK(m.applied_count == 0, "home: applies nothing");

    n = upd(&m, pad_buttons((uint16_t)(T89_PAD_HOME | T89_PAD_A)));
    CHECK(n == 1, "home: A beside HOME still presses");
    CHECK_EV(0, TIKEY_ENTER1, 1, "home: HOME does not shadow A");

    /* Still silent after the layer has moved on. */
    t89_keymap_reset(&m);
    upd(&m, pad_buttons(T89_PAD_ZL));
    upd(&m, pad_buttons(0));
    CHECK(m.layer == 1, "home: (setup) layer is digits");
    n = upd(&m, pad_buttons(T89_PAD_HOME));
    CHECK(n == 0, "home: presses no key in digits");
    CHECK(m.applied_count == 0, "home: applies nothing in digits");
}

static void test_zl_cycles_layers(void)
{
    t89_keymap_t m;

    t89_keymap_reset(&m);

    CHECK(upd(&m, pad_buttons(T89_PAD_ZL)) == 0, "zl: rising edge emits no key");
    CHECK(m.layer == 1, "zl: 0 -> 1");
    CHECK(m.applied_count == 0, "zl: applies no key while cycling");

    CHECK(upd(&m, pad_buttons(T89_PAD_ZL)) == 0, "zl: held ZL is silent");
    CHECK(m.layer == 1, "zl: held ZL does not cycle again");
    CHECK(upd(&m, pad_buttons(0)) == 0, "zl: release emits no key");

    upd(&m, pad_buttons(T89_PAD_ZL));
    CHECK(m.layer == 2, "zl: 1 -> 2");
    upd(&m, pad_buttons(0));
    CHECK(upd(&m, pad_buttons(T89_PAD_ZL)) == 0, "zl: third edge emits no key");
    CHECK(m.layer == 0, "zl: 2 -> 0");
}

static void test_layer_change_with_held_button(void)
{
    t89_keymap_t m;
    int n;

    t89_keymap_reset(&m);
    n = upd(&m, pad_buttons(T89_PAD_A));
    CHECK(n == 1 && g_ev[0].key == TIKEY_ENTER1, "layer: (setup) nav A is ENTER1");

    /* A held while ZL rises: nav ENTER1 -> digits TIKEY_5 in one update. */
    n = upd(&m, pad_buttons((uint16_t)(T89_PAD_A | T89_PAD_ZL)));
    CHECK(n == 2, "layer: held button re-maps as release plus press");
    CHECK_EV(0, TIKEY_ENTER1, 0, "layer: old key released first");
    CHECK_EV(1, TIKEY_5, 1, "layer: new key pressed second");
    CHECK(m.layer == 1, "layer: advanced to digits");
    CHECK(m.applied_count == 1, "layer: exactly one key applied after the move");

    /* On to math: digits TIKEY_5 -> math TIKEY_PLUS. */
    upd(&m, pad_buttons(T89_PAD_A));
    n = upd(&m, pad_buttons((uint16_t)(T89_PAD_A | T89_PAD_ZL)));
    CHECK(n == 2, "layer: second move also re-maps");
    CHECK_EV(0, TIKEY_5, 0, "layer: digits key released");
    CHECK_EV(1, TIKEY_PLUS, 1, "layer: math key pressed");
    CHECK(m.layer == 2, "layer: advanced to math");
    CHECK(m.applied_count == 1, "layer: still one key applied");
}

static void test_event_order(void)
{
    t89_keymap_t m;
    int n;

    t89_keymap_reset(&m);

    /* nav: A, X, Y -> ENTER1, 2ND, DIAMOND (6, 7, 76). */
    n = upd(&m, pad_buttons((uint16_t)(T89_PAD_A | T89_PAD_X | T89_PAD_Y)));
    CHECK(n == 3, "order: three new keys, three presses");
    CHECK_EV(0, TIKEY_DIAMOND, 1, "order: presses ascend, DIAMOND first");
    CHECK_EV(1, TIKEY_2ND, 1, "order: presses ascend, 2ND second");
    CHECK_EV(2, TIKEY_ENTER1, 1, "order: presses ascend, ENTER1 last");

    /* Swap all three for L and R -> ALPHA (79) and MODE (64). */
    n = upd(&m, pad_buttons((uint16_t)(T89_PAD_L | T89_PAD_R)));
    CHECK(n == 5, "order: three releases then two presses");
    CHECK_EV(0, TIKEY_DIAMOND, 0, "order: releases ascend, DIAMOND first");
    CHECK_EV(1, TIKEY_2ND, 0, "order: releases ascend, 2ND second");
    CHECK_EV(2, TIKEY_ENTER1, 0, "order: releases ascend, ENTER1 last");
    CHECK_EV(3, TIKEY_MODE, 1, "order: presses after releases, MODE first");
    CHECK_EV(4, TIKEY_ALPHA, 1, "order: presses after releases, ALPHA last");
}

static void test_event_limit(void)
{
    t89_keymap_t m;
    int n;

    /* Six changes do not fit in four events: nothing is written. */
    t89_keymap_reset(&m);
    upd(&m, pad_buttons((uint16_t)(T89_PAD_A | T89_PAD_X | T89_PAD_Y)));
    CHECK(m.applied_count == 3, "limit: (setup) three keys applied");

    n = upd_ev(&m, pad_buttons((uint16_t)(T89_PAD_B | T89_PAD_L | T89_PAD_R)), 4);
    CHECK(n == -1, "limit: over-long difference returns -1");
    CHECK(m.applied_count == 3, "limit: applied_count unchanged after -1");
    CHECK(m.layer == 0, "limit: layer unchanged after -1");

    /* The map really is untouched: the old keys are still applied. */
    n = upd(&m, pad_buttons(0));
    CHECK(n == 3, "limit: old keys are still applied");
    CHECK_EV(0, TIKEY_DIAMOND, 0, "limit: unchanged key DIAMOND released");
    CHECK_EV(1, TIKEY_2ND, 0, "limit: unchanged key 2ND released");
    CHECK_EV(2, TIKEY_ENTER1, 0, "limit: unchanged key ENTER1 released");

    /* A rejected update must not commit its layer change either. */
    t89_keymap_reset(&m);
    upd(&m, pad_buttons((uint16_t)(T89_PAD_A | T89_PAD_X | T89_PAD_Y)));
    n = upd_ev(&m, pad_buttons((uint16_t)(T89_PAD_A | T89_PAD_X | T89_PAD_Y | T89_PAD_ZL)), 4);
    CHECK(n == -1, "limit: layer change over the limit returns -1");
    CHECK(m.layer == 0, "limit: rejected layer change is not committed");
    CHECK(m.applied_count == 3, "limit: keys unchanged when the move is rejected");

    /* Exactly fitting the buffer is fine. */
    t89_keymap_reset(&m);
    upd(&m, pad_buttons((uint16_t)(T89_PAD_A | T89_PAD_X)));
    n = upd_ev(&m, pad_buttons((uint16_t)(T89_PAD_B | T89_PAD_L)), 4);
    CHECK(n == 4, "limit: an exactly-fitting difference is applied");
    CHECK_EV(0, TIKEY_2ND, 0, "limit: exact fit releases ascend (2ND)");
    CHECK_EV(1, TIKEY_ENTER1, 0, "limit: exact fit releases ascend (ENTER1)");
    CHECK_EV(2, TIKEY_ESCAPE, 1, "limit: exact fit presses ascend (ESCAPE)");
    CHECK_EV(3, TIKEY_ALPHA, 1, "limit: exact fit presses ascend (ALPHA)");
    CHECK(m.applied_count == 2, "limit: exact fit applies both new keys");

    /* No room and no change is still fine. */
    t89_keymap_reset(&m);
    CHECK(upd_ev(&m, pad_buttons(0), 0) == 0, "limit: empty difference fits in zero");
    CHECK(upd_ev(&m, pad_buttons(T89_PAD_A), 0) == -1, "limit: no room returns -1");
    CHECK(m.applied_count == 0, "limit: still nothing applied");
}

static void test_stick_and_directions(void)
{
    t89_keymap_t m;
    int n;

    t89_keymap_reset(&m);

    /* Dead zone edges: 19 is out, 20 is in, 44 is in, 45 is out. */
    n = upd(&m, mkpad(0, 19, 32));
    CHECK(n == 1, "stick: x=19 presses a direction");
    CHECK_EV(0, TIKEY_LEFT, 1, "stick: x=19 is LEFT");
    upd(&m, mkpad(0, 32, 32));

    n = upd(&m, mkpad(0, 20, 32));
    CHECK(n == 0, "stick: x=20 is inside the dead zone");
    n = upd(&m, mkpad(0, 44, 32));
    CHECK(n == 0, "stick: x=44 is inside the dead zone");

    n = upd(&m, mkpad(0, 45, 32));
    CHECK(n == 1, "stick: x=45 presses a direction");
    CHECK_EV(0, TIKEY_RIGHT, 1, "stick: x=45 is RIGHT");
    upd(&m, mkpad(0, 32, 32));

    n = upd(&m, mkpad(0, 32, 19));
    CHECK(n == 1, "stick: y=19 presses a direction");
    CHECK_EV(0, TIKEY_UP, 1, "stick: y=19 is UP");
    upd(&m, mkpad(0, 32, 32));

    n = upd(&m, mkpad(0, 32, 20));
    CHECK(n == 0, "stick: y=20 is inside the dead zone");
    n = upd(&m, mkpad(0, 32, 44));
    CHECK(n == 0, "stick: y=44 is inside the dead zone");

    n = upd(&m, mkpad(0, 32, 45));
    CHECK(n == 1, "stick: y=45 presses a direction");
    CHECK_EV(0, TIKEY_DOWN, 1, "stick: y=45 is DOWN");
    upd(&m, mkpad(0, 32, 32));

    /* Opposites cancel, whether both come from the D-pad or one from each. */
    n = upd(&m, pad_buttons((uint16_t)(T89_PAD_UP | T89_PAD_DOWN)));
    CHECK(n == 0, "cancel: D-pad UP+DOWN press nothing");
    n = upd(&m, pad_buttons((uint16_t)(T89_PAD_LEFT | T89_PAD_RIGHT)));
    CHECK(n == 0, "cancel: D-pad LEFT+RIGHT press nothing");
    n = upd(&m, mkpad(T89_PAD_UP, 32, 45));
    CHECK(n == 0, "cancel: D-pad UP against stick DOWN presses nothing");
    n = upd(&m, mkpad(T89_PAD_LEFT, 45, 32));
    CHECK(n == 0, "cancel: D-pad LEFT against stick RIGHT presses nothing");
    CHECK(m.applied_count == 0, "cancel: nothing applied while cancelling");

    /* D-pad and stick agreeing is still a single key. */
    n = upd(&m, mkpad(T89_PAD_LEFT, 19, 32));
    CHECK(n == 1, "merge: D-pad and stick agree on one key");
    CHECK_EV(0, TIKEY_LEFT, 1, "merge: LEFT pressed once, not twice");
    n = upd(&m, mkpad(0, 32, 32));
    CHECK(n == 1, "merge: LEFT released once");
    CHECK_EV(0, TIKEY_LEFT, 0, "merge: release is LEFT");

    /* Directions follow the layer, including for the stick. */
    t89_keymap_reset(&m);
    upd(&m, pad_buttons(T89_PAD_ZL));
    upd(&m, pad_buttons(0));
    n = upd(&m, mkpad(0, 19, 32));
    CHECK(n == 1, "digits: stick LEFT presses a key");
    CHECK_EV(0, TIKEY_4, 1, "digits: stick LEFT is TIKEY_4");
    upd(&m, mkpad(0, 32, 32));
    n = upd(&m, pad_buttons(T89_PAD_UP));
    CHECK(n == 1, "digits: D-pad UP presses a key");
    CHECK_EV(0, TIKEY_8, 1, "digits: D-pad UP is TIKEY_8");

    /* nav, two buttons at once, to show SELECT/START in place. */
    t89_keymap_reset(&m);
    n = upd(&m, pad_buttons((uint16_t)(T89_PAD_START | T89_PAD_SELECT)));
    CHECK(n == 2, "nav: two buttons give two presses");
    CHECK_EV(0, TIKEY_APPS, 1, "nav: SELECT is TIKEY_APPS");
    CHECK_EV(1, TIKEY_HOME, 1, "nav: START is TIKEY_HOME");

    /* math layer through the public path. */
    t89_keymap_reset(&m);
    upd(&m, pad_buttons(T89_PAD_ZL));
    upd(&m, pad_buttons(0));
    upd(&m, pad_buttons(T89_PAD_ZL));
    upd(&m, pad_buttons(0));
    CHECK(m.layer == 2, "math: (setup) layer is math");
    n = upd(&m, pad_buttons((uint16_t)(T89_PAD_A | T89_PAD_B | T89_PAD_X | T89_PAD_Y)));
    CHECK(n == 4, "math: four face buttons give four keys");
    CHECK_EV(0, TIKEY_DIVIDE, 1, "math: Y is TIKEY_DIVIDE");
    CHECK_EV(1, TIKEY_MULTIPLY, 1, "math: X is TIKEY_MULTIPLY");
    CHECK_EV(2, TIKEY_PLUS, 1, "math: A is TIKEY_PLUS");
    CHECK_EV(3, TIKEY_MINUS, 1, "math: B is TIKEY_MINUS");
}

/*
 * No controller.  The app must pass t89_pad_none() rather than a zeroed
 * struct or an undecoded register read: those decode to the stick at an
 * extreme, which the direction pass reads as a direction held.  These checks
 * pin both the correct path and the trap that makes it necessary.
 */
static void test_no_pad(void)
{
    t89_keymap_t m;
    t89_pad_t p, none;
    int n;

    t89_pad_none(&none);
    CHECK(none.buttons == 0, "none: presses nothing");
    CHECK(none.left_x == 32 && none.left_y == 32, "none: both axes centred");

    /* The absent controller's registers read all ones, which decodes to the
     * stick at its maximum (down-right). */
    t89_pad_decode(0xffffffffu, 0xffffffffu, &p);
    CHECK(p.buttons == 0 && p.left_x == 63 && p.left_y == 63,
          "none: an absent register read decodes to the stick at max");

    /* A fresh map with a none pad applies nothing at all. */
    t89_keymap_reset(&m);
    n = upd(&m, none);
    CHECK(n == 0, "none: a fresh map emits no events");
    CHECK(m.applied_count == 0, "none: a fresh map applies no keys");

    /* The trap: a zeroed pad and an undecoded absent read each hold a
     * direction.  This is why t89_pad_none() exists. */
    t89_keymap_reset(&m);
    n = upd(&m, mkpad(0, 0, 0));
    CHECK(n == 2, "none: a zeroed pad holds up and left (the trap)");
    CHECK_EV(0, TIKEY_UP, 1, "none: zeroed pad presses TIKEY_UP");
    CHECK_EV(1, TIKEY_LEFT, 1, "none: zeroed pad presses TIKEY_LEFT");

    t89_keymap_reset(&m);
    t89_pad_decode(0xffffffffu, 0xffffffffu, &p);
    n = upd(&m, p);
    CHECK(n == 2, "none: an absent register read holds down and right");
    CHECK_EV(0, TIKEY_DOWN, 1, "none: absent read presses TIKEY_DOWN");
    CHECK_EV(1, TIKEY_RIGHT, 1, "none: absent read presses TIKEY_RIGHT");

    /* Unplugging releases what was held: press keys, then go to no pad. */
    t89_keymap_reset(&m);
    n = upd(&m, pad_buttons((uint16_t)(T89_PAD_A | T89_PAD_UP)));
    CHECK(n == 2, "none: (setup) A and UP press two keys");
    n = upd(&m, none);
    CHECK(n == 2, "none: unplugging releases both");
    CHECK_EV(0, TIKEY_UP, 0, "none: unplugging releases TIKEY_UP");
    CHECK_EV(1, TIKEY_ENTER1, 0, "none: unplugging releases TIKEY_ENTER1");
    CHECK(m.applied_count == 0, "none: after unplugging nothing is applied");
    CHECK(upd(&m, none) == 0, "none: a settled no-pad emits nothing");
}

int main(void)
{
    test_decode();
    test_layer_name();
    test_layer_keys();
    test_layer_directions();
    test_press_and_release();
    test_home_silence();
    test_zl_cycles_layers();
    test_layer_change_with_held_button();
    test_event_order();
    test_event_limit();
    test_stick_and_directions();
    test_no_pad();

    printf("t89_pad: %d checks passed\n", g_checks - g_fails);
    return (g_fails == 0 && g_checks >= 25) ? 0 : 1;
}
