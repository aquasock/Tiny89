/* SPDX-License-Identifier: MIT */
/*
 * The keyboard mapping's host test: the register split, the usage table, and
 * the rule that a report is state rather than events.  No board and no
 * toolchain; see tests/run_t89_key_test.sh.
 */
#include <stdio.h>
#include <string.h>

#include "keydefs.h"
#include "t89_key.h"

static int g_checks, g_fails;

static void check(int cond, const char *name)
{
    g_checks++;
    if (!cond) {
        g_fails++;
        printf("FAIL %s\n", name);
    }
}

static t89_kbd_t mk(uint8_t k0, uint8_t k1)
{
    t89_kbd_t k;

    memset(&k, 0, sizeof k);
    k.keys[0] = k0;
    k.keys[1] = k1;
    return k;
}

/* The events the last update produced. */
#define NEV 16
static t89_key_event_t g_ev[NEV];
static int g_n;

static int upd(const t89_kbd_t *k)
{
    g_n = t89_kbd_update(k, g_ev, NEV);
    return g_n;
}

static int has_key(int key, int active)
{
    int i;

    for (i = 0; i < g_n; i++) {
        if (g_ev[i].key == key && g_ev[i].active == active) return 1;
    }
    return 0;
}

int main(void)
{
    t89_kbd_t k, decoded;
    uint8_t seq = 0;

    /* The fabric's two registers: kbd_lo {u3, u2, u1, mods}, kbd_hi
     * {sequence, u6, u5, u4}. */
    t89_kbd_decode(0x03020105u, 0x7f060504u, &decoded, &seq);
    check(decoded.mods == 0x05, "decode: the modifier byte");
    check(decoded.keys[0] == 0x01 && decoded.keys[1] == 0x02 &&
          decoded.keys[2] == 0x03, "decode: the low register's three usages");
    check(decoded.keys[3] == 0x04 && decoded.keys[4] == 0x05 &&
          decoded.keys[5] == 0x06, "decode: the high register's three usages");
    check(seq == 0x7f, "decode: the sequence byte");

    /* The mapping names what it should and nothing else. */
    check(t89_kbd_key(0x52) == TIKEY_UP, "map: 0x52 is up");
    check(t89_kbd_key(0x51) == TIKEY_DOWN, "map: 0x51 is down");
    check(t89_kbd_key(0x28) == TIKEY_ENTER1, "map: enter");
    check(t89_kbd_key(0x29) == TIKEY_ESCAPE, "map: escape");
    check(t89_kbd_key(0x2A) == TIKEY_BACKSPACE, "map: backspace");
    check(t89_kbd_key(0x2B) == TIKEY_2ND, "map: tab is 2nd");
    check(t89_kbd_key(0x3A) == TIKEY_DIAMOND, "map: F1 is the diamond");
    check(t89_kbd_key(0x1F) == TIKEY_2, "map: the number row");
    check(t89_kbd_key(0x27) == TIKEY_0, "map: zero");
    check(t89_kbd_key(0x1D) == TIKEY_Z, "map: the letters");
    check(t89_kbd_key(0x55) == TIKEY_MULTIPLY, "map: keypad star");
    check(t89_kbd_key(0x38) == TIKEY_DIVIDE, "map: slash");
    check(t89_kbd_key(0x00) == -1, "map: nothing is not a key");
    check(t89_kbd_key(0xF0) == -1, "map: an unnamed usage is nothing");

    /* A report is state: holding a key is one press, and letting go is one
     * release. */
    t89_kbd_reset();
    k = mk(0x52, 0x00);
    check(upd(&k) == 1, "state: one key down is one press");
    check(has_key(TIKEY_UP, 1), "state: and it is the up key");
    check(upd(&k) == 0, "state: holding it emits nothing more");
    k = mk(0x52, 0x28);
    check(upd(&k) == 1, "state: a second key is one more press");
    check(has_key(TIKEY_ENTER1, 1), "state: and it is enter");
    k = mk(0x00, 0x00);
    check(upd(&k) == 2, "state: releasing both is two releases");
    check(has_key(TIKEY_UP, 0) && has_key(TIKEY_ENTER1, 0),
          "state: both releases are reported");
    check(upd(&k) == 0, "state: an unchanged empty report is silent");

    /* An unnamed key presses nothing, and the six slots are all read. */
    t89_kbd_reset();
    k = mk(0x52, 0x00);
    k.keys[5] = 0x28;
    check(upd(&k) == 2, "state: the sixth slot is used");
    check(has_key(TIKEY_ENTER1, 1), "state: and it is mapped");

    /* The difference has to fit. */
    t89_kbd_reset();
    k = mk(0x04, 0x05);
    k.keys[2] = 0x06;
    k.keys[3] = 0x07;
    k.keys[4] = 0x08;
    k.keys[5] = 0x09;
    g_n = t89_kbd_update(&k, g_ev, 4);
    check(g_n == -1, "state: a difference that does not fit is refused");
    g_n = t89_kbd_update(&k, g_ev, NEV);
    check(g_n == 6, "state: and it applies once it fits");
    check(has_key(TIKEY_A, 1) && has_key(TIKEY_F, 1),
          "state: every key in the report is pressed");

    if (g_fails == 0)
        printf("t89_key: %d checks passed\n", g_checks);
    else
        printf("t89_key: %d FAILURES among %d checks\n", g_fails, g_checks);
    return (g_fails == 0) ? 0 : 1;
}
