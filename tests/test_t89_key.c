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
    check(t89_kbd_key(0x50) == TIKEY_LEFT, "map: 0x50 is left");
    check(t89_kbd_key(0x4F) == TIKEY_RIGHT, "map: 0x4f is right");
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

    /* Every binding, with the usage's name from HID Usage Tables for USB 1.4
     * section 10 (core-reference.md, KEY-001).  Carrying the name is the
     * point: the two horizontal arrows were swapped for two cycles and the
     * host test missed it because it checked up and down only.  A usage that
     * is mistyped, dropped or moved now fails here by name. */
    {
        static const struct { uint8_t usage; const char *name; int key; }
        binds[] = {
            { 0x52, "UpArrow", TIKEY_UP },
            { 0x51, "DownArrow", TIKEY_DOWN },
            { 0x50, "LeftArrow", TIKEY_LEFT },
            { 0x4F, "RightArrow", TIKEY_RIGHT },
            { 0x28, "Return (ENTER)", TIKEY_ENTER1 },
            { 0x29, "ESCAPE", TIKEY_ESCAPE },
            { 0x2A, "DELETE (Backspace)", TIKEY_BACKSPACE },
            { 0x2B, "Tab", TIKEY_2ND },
            { 0x3A, "F1", TIKEY_DIAMOND },
            { 0x3B, "F2", TIKEY_ALPHA },
            { 0x3C, "F3", TIKEY_MODE },
            { 0x3D, "F4", TIKEY_CATALOG },
            { 0x1E, "1 and !", TIKEY_1 }, { 0x1F, "2 and @", TIKEY_2 },
            { 0x20, "3 and #", TIKEY_3 }, { 0x21, "4 and $", TIKEY_4 },
            { 0x22, "5 and %", TIKEY_5 }, { 0x23, "6 and ^", TIKEY_6 },
            { 0x24, "7 and &", TIKEY_7 }, { 0x25, "8 and *", TIKEY_8 },
            { 0x26, "9 and (", TIKEY_9 }, { 0x27, "0 and )", TIKEY_0 },
            { 0x04, "a and A", TIKEY_A }, { 0x05, "b and B", TIKEY_B },
            { 0x06, "c and C", TIKEY_C }, { 0x07, "d and D", TIKEY_D },
            { 0x08, "e and E", TIKEY_E }, { 0x09, "f and F", TIKEY_F },
            { 0x0A, "g and G", TIKEY_G }, { 0x0B, "h and H", TIKEY_H },
            { 0x0C, "i and I", TIKEY_I }, { 0x0D, "j and J", TIKEY_J },
            { 0x0E, "k and K", TIKEY_K }, { 0x0F, "l and L", TIKEY_L },
            { 0x10, "m and M", TIKEY_M }, { 0x11, "n and N", TIKEY_N },
            { 0x12, "o and O", TIKEY_O }, { 0x13, "p and P", TIKEY_P },
            { 0x14, "q and Q", TIKEY_Q }, { 0x15, "r and R", TIKEY_R },
            { 0x16, "s and S", TIKEY_S }, { 0x17, "t and T", TIKEY_T },
            { 0x18, "u and U", TIKEY_U }, { 0x19, "v and V", TIKEY_V },
            { 0x1A, "w and W", TIKEY_W }, { 0x1B, "x and X", TIKEY_X },
            { 0x1C, "y and Y", TIKEY_Y }, { 0x1D, "z and Z", TIKEY_Z },
            { 0x2D, "- and _", TIKEY_MINUS },
            { 0x2E, "= and +", TIKEY_EQUALS },
            { 0x2F, "[ and {", TIKEY_PALEFT },
            { 0x30, "] and }", TIKEY_PARIGHT },
            { 0x33, "; and :", TIKEY_COMMA },
            { 0x34, "' and \"", TIKEY_PIPE },
            { 0x36, ", and <", TIKEY_COMMA },
            { 0x37, ". and >", TIKEY_PERIOD },
            { 0x38, "/ and ?", TIKEY_DIVIDE },
            { 0x54, "Keypad /", TIKEY_DIVIDE },
            { 0x55, "Keypad *", TIKEY_MULTIPLY },
            { 0x56, "Keypad -", TIKEY_MINUS },
            { 0x57, "Keypad +", TIKEY_PLUS },
            { 0x58, "Keypad ENTER", TIKEY_ENTER2 },
            { 0x63, "Keypad .", TIKEY_PERIOD },
            { 0x2C, "Spacebar", TIKEY_SPACE },
            { 0x39, "Caps Lock", TIKEY_HAND },
            { 0x49, "Insert", TIKEY_APPS },
            { 0x4A, "Home", TIKEY_HOME },
            { 0x4B, "PageUp", TIKEY_STORE },
            { 0x4C, "Delete Forward", TIKEY_CLEAR },
            { 0x4D, "End", TIKEY_ON },
            { 0x4E, "PageDown", TIKEY_EE },
            { 0x3F, "F6", TIKEY_SIN },
            { 0x40, "F7", TIKEY_COS },
            { 0x41, "F8", TIKEY_TAN },
            { 0x42, "F9", TIKEY_LN },
            { 0x43, "F10", TIKEY_NU },
            { 0x44, "F11", TIKEY_THETA }
        };
        static const struct { uint8_t usage; const char *name; int key; }
        shifted[] = {
            { 0x2E, "= and +", TIKEY_PLUS },
            { 0x25, "8 and *", TIKEY_MULTIPLY },
            { 0x23, "6 and ^", TIKEY_POWER },
            { 0x2D, "- and _", TIKEY_NEGATE },
            { 0x3A, "F1", TIKEY_F1 }, { 0x3B, "F2", TIKEY_F2 },
            { 0x3C, "F3", TIKEY_F3 }, { 0x3D, "F4", TIKEY_F4 },
            { 0x3E, "F5", TIKEY_F5 }, { 0x3F, "F6", TIKEY_F6 },
            { 0x40, "F7", TIKEY_F7 }, { 0x41, "F8", TIKEY_F8 }
        };
        char name[64];
        size_t i;

        for (i = 0; i < sizeof binds / sizeof binds[0]; i++) {
            snprintf(name, sizeof name, "map: 0x%02x is %s",
                     binds[i].usage, binds[i].name);
            check(t89_kbd_key(binds[i].usage) == binds[i].key, name);
        }
        for (i = 0; i < sizeof shifted / sizeof shifted[0]; i++) {
            snprintf(name, sizeof name, "shift map: 0x%02x is %s held",
                     shifted[i].usage, shifted[i].name);
            check(t89_kbd_shift_key(shifted[i].usage) == shifted[i].key, name);
        }
    }
    check(t89_kbd_shift_key(0x00) == -1, "shift map: nothing is not a key");
    check(t89_kbd_shift_key(0x52) == -1,
          "shift map: a key Shift does not name has no shifted meaning");

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

    /* Shift picks between the two tables in implied(), and a usage the shift
     * table does not name keeps the meaning it has unshifted. */
    t89_kbd_reset();
    k = mk(0x2E, 0x00);                 /* the equals key */
    k.mods = 0x02;                      /* LeftShift */
    check(upd(&k) == 1, "shift: Shift with equals is one press");
    check(has_key(TIKEY_PLUS, 1), "shift: the calculator sees plus");
    check(!has_key(TIKEY_EQUALS, 1), "shift: and not equals as well");
    k.mods = 0x20;                      /* RightShift */
    check(upd(&k) == 0, "shift: either Shift bit presses the same key");
    k.mods = 0x00;
    check(upd(&k) == 2, "shift: letting Shift go is a release and a press");
    check(has_key(TIKEY_PLUS, 0) && has_key(TIKEY_EQUALS, 1),
          "shift: plus released and equals pressed");

    t89_kbd_reset();
    k = mk(0x52, 0x3F);                 /* Up and F6 */
    k.mods = 0x02;
    check(upd(&k) == 2, "shift: a shifted report is two presses");
    check(has_key(TIKEY_F6, 1), "shift: F6 held is the calculator's F6");
    check(!has_key(TIKEY_SIN, 1), "shift: and not sin");
    check(has_key(TIKEY_UP, 1), "shift: a key outside the shift table is itself");

    /* Control is the program's, not the calculator's: held with a usage it
     * names a command, and nothing is mapped from such a report. */
    {
        t89_kbd_t c;

        memset(&c, 0, sizeof c);
        c.mods = 0x01; c.keys[0] = 0x16;          /* LeftControl, the s key */
        check(t89_kbd_command(&c) == T89_KBD_CMD_SAVE,
              "cmd: Control with s is save");
        c.mods = 0x10;                            /* RightControl */
        check(t89_kbd_command(&c) == T89_KBD_CMD_SAVE,
              "cmd: either Control bit does it");
        c.mods = 0x00;
        check(t89_kbd_command(&c) == T89_KBD_CMD_NONE,
              "cmd: s on its own is not a command");
        c.mods = 0x01; c.keys[0] = 0x04;          /* Control with a */
        check(t89_kbd_command(&c) == T89_KBD_CMD_NONE,
              "cmd: another key is not save");
        c.mods = 0x02; c.keys[0] = 0x16;          /* Shift with s */
        check(t89_kbd_command(&c) == T89_KBD_CMD_NONE,
              "cmd: Shift with s is not a command");
        c.mods = 0x05; c.keys[0] = 0x14;          /* Control and Alt with q */
        check(t89_kbd_command(&c) == T89_KBD_CMD_EXIT,
              "cmd: Control and Alt with q is the exit");
        c.mods = 0x50; c.keys[0] = 0x14;          /* RightControl, RightAlt */
        check(t89_kbd_command(&c) == T89_KBD_CMD_EXIT,
              "cmd: either Alt bit with either Control bit does it");
        c.mods = 0x44; c.keys[0] = 0x14;          /* both Alt bits, no Control */
        check(t89_kbd_command(&c) == T89_KBD_CMD_NONE,
              "cmd: Alt without Control is not the exit");
        c.mods = 0x01; c.keys[0] = 0x14;          /* Control with q alone */
        check(t89_kbd_command(&c) == T89_KBD_CMD_NONE,
              "cmd: Control without Alt is not the exit");
        c.mods = 0x04; c.keys[0] = 0x14;          /* Alt with q alone */
        check(t89_kbd_command(&c) == T89_KBD_CMD_NONE,
              "cmd: Alt without Control is not the exit");
        c.mods = 0x05; c.keys[0] = 0x16;          /* Control and Alt with s */
        check(t89_kbd_command(&c) == T89_KBD_CMD_NONE,
              "cmd: Alt with Control and s is neither command");
        check(t89_kbd_command(NULL) == T89_KBD_CMD_NONE,
              "cmd: no report, no command");
    }

    t89_kbd_reset();
    k = mk(0x16, 0x00);                 /* the s key */
    k.mods = 0x01;                      /* held with Control */
    check(upd(&k) == 0, "cmd: Control with s presses nothing on the calculator");
    k.mods = 0x00;
    check(upd(&k) == 1, "cmd: letting Control go presses s");
    check(has_key(TIKEY_S, 1), "cmd: and the key is s");
    check(upd(&k) == 0, "cmd: and holding it emits nothing more");
    k = mk(0x00, 0x00);
    check(upd(&k) == 1, "cmd: releasing it is the one release");
    check(has_key(TIKEY_S, 0), "cmd: and it is the s key that went up");

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
