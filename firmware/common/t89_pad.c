/* SPDX-License-Identifier: MIT */
/*
 * Wii Classic Controller -> TI-89 keypad mapping for Tiny89.
 *
 * Tiny89 runs on the inherited TinyNES AE350 SoC fabric (TinyNES commit
 * f5011dd374cfc65f4f17616fec97c903a66b6cc8, fpga/ae350_soc/wii_pads.sv).  The
 * Wii Classic Controller unit is at 0xe8040000: its DATA(n) register returns
 * report bytes 0..3 and DATA2(n) bits 15:0 returns bytes 4 and 5.
 *
 * The button word is the one TinyNES firmware/apps/pad_test/main.c decodes,
 * validated on this board: the fifteen active-low buttons are
 * ~((byte5 << 8) | byte4), byte 4 in bits 7:0 -- bit 15 ZL, 14 B, 13 Y, 12 A,
 * 11 X, 10 ZR, 9 LEFT, 8 UP, 7 RIGHT, 6 DOWN, 5 L, 4 SELECT ('-'), 3 HOME,
 * 2 START ('+'), 1 R; bit 0 unused.  Axes: byte0 bits 5:0 = left X, byte1
 * bits 5:0 = left Y (6-bit, 32 = centre).
 *
 * HOME is consumed by the fabric (a tap toggles TinyDesk's overlay), so it is
 * unmapped in every layer here and must never reach the calculator.
 *
 * TI-89 key codes are TiEmu's TiKey enum, third_party/tiemu/tiemu/trunk/src/
 * core/ti_hw/keydefs.h (TiEmu pinned at 2b8df9280d2054e11ffcf7e57a73899b3ce7a7e5).
 * Enum names are used throughout; no raw key numbers appear below.
 */
#include "t89_pad.h"

/* Logical directions, in the order the tests and the API use them. */
#define T89_DIR_UP    0
#define T89_DIR_DOWN  1
#define T89_DIR_LEFT  2
#define T89_DIR_RIGHT 3
#define T89_DIR_COUNT 4

/* Left-stick dead zone: 12 counts around centre 32, so 20..44 is centred and
 * <= 19 / >= 45 are the extremes. */
#define T89_STICK_CENTRE 32
#define T89_STICK_DEAD   12
#define T89_STICK_LOW    (T89_STICK_CENTRE - T89_STICK_DEAD - 1)  /* 19 */
#define T89_STICK_HIGH   (T89_STICK_CENTRE + T89_STICK_DEAD + 1)  /* 45 */

/* One row per controller button: what that single bit means in every layer.
 * This table is the single place the mapping is written down; the lookups and
 * the update path are all driven from it.  A -1 means "unmapped": ZL only
 * cycles the layer and HOME belongs to the fabric. */
typedef struct {
    uint16_t button;
    int key[T89_LAYER_COUNT];
} t89_bind_t;

static const t89_bind_t t89_binds[] = {
    /* button         layer 0 'nav'    layer 1 'digits'   layer 2 'math'      */
    { T89_PAD_UP,     { TIKEY_UP,      TIKEY_8,          TIKEY_UP            } },
    { T89_PAD_DOWN,   { TIKEY_DOWN,    TIKEY_2,          TIKEY_DOWN          } },
    { T89_PAD_LEFT,   { TIKEY_LEFT,    TIKEY_4,          TIKEY_LEFT          } },
    { T89_PAD_RIGHT,  { TIKEY_RIGHT,   TIKEY_6,          TIKEY_RIGHT         } },
    { T89_PAD_A,      { TIKEY_ENTER1,  TIKEY_5,          TIKEY_PLUS          } },
    { T89_PAD_B,      { TIKEY_ESCAPE,  TIKEY_0,          TIKEY_MINUS         } },
    { T89_PAD_X,      { TIKEY_2ND,     TIKEY_1,          TIKEY_MULTIPLY      } },
    { T89_PAD_Y,      { TIKEY_DIAMOND, TIKEY_3,          TIKEY_DIVIDE        } },
    { T89_PAD_L,      { TIKEY_ALPHA,   TIKEY_9,          TIKEY_POWER         } },
    { T89_PAD_R,      { TIKEY_MODE,    TIKEY_7,          TIKEY_EQUALS        } },
    { T89_PAD_SELECT, { TIKEY_APPS,    TIKEY_NEGATE,     TIKEY_NEGATE        } },
    { T89_PAD_START,  { TIKEY_HOME,    TIKEY_ENTER1,     TIKEY_ENTER1        } },
    { T89_PAD_ZR,     { TIKEY_CATALOG, TIKEY_PERIOD,     TIKEY_PARIGHT       } },
    { T89_PAD_ZL,     { -1,            -1,               -1                  } },
    { T89_PAD_HOME,   { -1,            -1,               -1                  } }
};

#define T89_BIND_COUNT (sizeof(t89_binds) / sizeof(t89_binds[0]))

/* Which D-pad bit carries each logical direction. */
static const uint16_t t89_dir_button[T89_DIR_COUNT] = {
    T89_PAD_UP, T89_PAD_DOWN, T89_PAD_LEFT, T89_PAD_RIGHT
};

/*
 * The previous ZL level, needed to detect its rising edge, is latched in the
 * top slot of map->applied[].  TiKey only spans 0..MAX_TIKEYS-1 (84) while
 * applied[] is NB_MAX_KEYS (120) entries, so that slot is not a key index and
 * the assert below keeps it that way.  t89_keymap_t is a frozen interface, so
 * the latch has to live inside it; it is never reported as a key or counted by
 * applied_count.
 */
#define T89_ZL_LATCH (NB_MAX_KEYS - 1)
_Static_assert(MAX_TIKEYS <= T89_ZL_LATCH,
               "a TiKey must never collide with the ZL latch slot");

void t89_pad_decode(uint32_t lo, uint32_t hi, t89_pad_t *out)
{
    if (!out) return;

    /* DATA(n) = report bytes 0..3, DATA2(n) bits 15:0 = bytes 4 and 5.  The
     * fifteen active-low buttons live in bytes 4 and 5; bit 0 is unused. */
    out->buttons = (uint16_t)(~hi) & 0xfffeu;
    out->left_x = (uint8_t)(lo & 0x3fu);         /* byte 0 bits 5:0 */
    out->left_y = (uint8_t)((lo >> 8) & 0x3fu);  /* byte 1 bits 5:0 */
}

/*
 * This is raw: every word decodes to some pad state, and both words an absent
 * controller can leave behind decode to *something*.  With nothing plugged in
 * the unit's registers read all ones, which puts both axes at their maximum
 * (down-right), and a freshly zeroed register puts them at their minimum
 * (up-left); either way the stick reads as a direction held.  A caller that
 * has no controller must therefore not decode at all: it passes the centred,
 * buttonless pad t89_pad_none() returns, so nothing is ever pressed.
 *
 * t89_pad_none() is the one place "no controller" is written down.
 */
void t89_pad_none(t89_pad_t *out)
{
    if (!out) return;

    out->buttons = 0;
    out->left_x = (uint8_t)T89_STICK_CENTRE;
    out->left_y = (uint8_t)T89_STICK_CENTRE;
}

void t89_keymap_reset(t89_keymap_t *map)
{
    int i;

    if (!map) return;

    map->layer = 0;
    map->applied_count = 0;
    for (i = 0; i < NB_MAX_KEYS; i++) map->applied[i] = 0;
}

const char *t89_layer_name(int layer)
{
    switch (layer) {
    case 0: return "nav";
    case 1: return "digits";
    case 2: return "math";
    default: return 0;
    }
}

int t89_layer_key(int layer, uint16_t button)
{
    int i;

    if (layer < 0 || layer >= T89_LAYER_COUNT) return -1;
    if (button == 0) return -1;

    for (i = 0; i < (int)T89_BIND_COUNT; i++) {
        if (t89_binds[i].button == button) return t89_binds[i].key[layer];
    }
    return -1;  /* not a single button bit: unknown, multi-bit or bit 0 */
}

int t89_layer_direction(int layer, int direction)
{
    if (direction < 0 || direction >= T89_DIR_COUNT) return -1;
    return t89_layer_key(layer, t89_dir_button[direction]);
}

/* The four logical directions: the D-pad OR the left stick, with opposites
 * that are both set cancelling each other out. */
static void t89_directions(const t89_pad_t *pad, int dir[T89_DIR_COUNT])
{
    int up = (pad->buttons & T89_PAD_UP) != 0 || pad->left_y <= T89_STICK_LOW;
    int down = (pad->buttons & T89_PAD_DOWN) != 0 || pad->left_y >= T89_STICK_HIGH;
    int left = (pad->buttons & T89_PAD_LEFT) != 0 || pad->left_x <= T89_STICK_LOW;
    int right = (pad->buttons & T89_PAD_RIGHT) != 0 || pad->left_x >= T89_STICK_HIGH;

    if (up && down) up = down = 0;
    if (left && right) left = right = 0;

    dir[T89_DIR_UP] = up;
    dir[T89_DIR_DOWN] = down;
    dir[T89_DIR_LEFT] = left;
    dir[T89_DIR_RIGHT] = right;
}

/* The four D-pad bits are handled by t89_directions() (D-pad OR stick), so the
 * per-button pass below must skip them rather than re-mark a direction that
 * opposite cancellation has just cleared. */
static int t89_is_dir_button(uint16_t button)
{
    int i;

    for (i = 0; i < T89_DIR_COUNT; i++) {
        if (t89_dir_button[i] == button) return 1;
    }
    return 0;
}

static void t89_mark(unsigned char *set, int key)
{
    if (key >= 0 && key < NB_MAX_KEYS && key != T89_ZL_LATCH) set[key] = 1;
}

int t89_keymap_update(t89_keymap_t *map, const t89_pad_t *pad,
                      t89_key_event_t *events, int max_events)
{
    unsigned char desired[NB_MAX_KEYS];
    unsigned char current[NB_MAX_KEYS];
    int dir[T89_DIR_COUNT];
    int layer;
    int zl_now, zl_was;
    int releases = 0, presses = 0, applied = 0, n = 0;
    int i;

    if (!map || !pad || max_events < 0) return -1;

    layer = (map->layer >= 0 && map->layer < T89_LAYER_COUNT) ? map->layer : 0;

    /* ZL is a modifier: a rising edge advances 0 -> 1 -> 2 -> 0 and ZL itself
     * never emits a key.  The whole update is computed before anything is
     * committed, so a rejected update leaves the latch (and the layer) alone. */
    zl_now = (pad->buttons & T89_PAD_ZL) != 0;
    zl_was = map->applied[T89_ZL_LATCH] != 0;
    if (zl_now && !zl_was) layer = (layer + 1) % T89_LAYER_COUNT;

    for (i = 0; i < NB_MAX_KEYS; i++) {
        desired[i] = 0;
        current[i] = 0;
    }

    t89_directions(pad, dir);
    for (i = 0; i < T89_DIR_COUNT; i++) {
        if (dir[i]) t89_mark(desired, t89_layer_direction(layer, i));
    }

    for (i = 0; i < (int)T89_BIND_COUNT; i++) {
        if (t89_is_dir_button(t89_binds[i].button)) continue;
        if (pad->buttons & t89_binds[i].button) {
            t89_mark(desired, t89_layer_key(layer, t89_binds[i].button));
        }
    }

    /* Copy of what we last applied; the latch slot is not a key. */
    for (i = 0; i < NB_MAX_KEYS; i++) current[i] = (i == T89_ZL_LATCH) ? 0 : map->applied[i];

    for (i = 0; i < NB_MAX_KEYS; i++) {
        if (current[i] && !desired[i]) releases++;
        else if (!current[i] && desired[i]) presses++;
    }

    if (releases + presses > max_events) return -1;
    if (releases + presses > 0 && !events) return -1;

    /* Releases first, then presses, each group in ascending TiKey order. */
    for (i = 0; i < NB_MAX_KEYS; i++) {
        if (current[i] && !desired[i]) {
            events[n].key = i;
            events[n].active = 0;
            n++;
        }
    }
    for (i = 0; i < NB_MAX_KEYS; i++) {
        if (!current[i] && desired[i]) {
            events[n].key = i;
            events[n].active = 1;
            n++;
        }
    }

    /* Commit: the whole difference applied, including any layer change. */
    for (i = 0; i < NB_MAX_KEYS; i++) {
        map->applied[i] = desired[i];
        if (desired[i]) applied++;
    }
    map->applied[T89_ZL_LATCH] = (unsigned char)(zl_now ? 1 : 0);
    map->layer = layer;
    map->applied_count = applied;

    return n;
}
