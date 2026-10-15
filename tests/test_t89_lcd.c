/* SPDX-License-Identifier: MIT */
/*
 * Host tests for the Tiny89 grayscale LCD module: firmware/common/t89_lcd.c
 * (the binding to TiEmu's plane state) and firmware/common/t89_lcd_gray.c
 * (the composition ported from TiEmu's gui/calc/screen.c).
 *
 * Plain C, no framework, same shape as tests/test_t89_pad.c: every check is
 * one named comparison, mismatches print "FAIL <name>" and are counted.
 *
 * The expectations restate TiEmu's model on purpose -- gp_seq[3] = {0,0,1} for
 * the two-plane 4-colour case, gp_seq[1] = {0} for b&w -- rather than reading
 * the table back out of the module.  TiEmu's globals are defined here so the
 * module links without a calculator: ngc and lcd_planebufs[].
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "t89_lcd.h"
#include "t89_lcd_gray.h"

/* TiEmu's plane state, normally set by core/ti_hw/gscales.c (lcd_planebufs[3]
 * and ngc), declared the same way t89_lcd.c does. */
int ngc = 1;
uint8_t *lcd_planebufs[3] = { NULL, NULL, NULL };

#define OUT_ROW_BYTES 256u
#define OUT_BYTES (OUT_ROW_BYTES * 100u)
#define GUARD_BYTES 512u
#define PLANE_BYTES (128u * 30u)

static uint8_t g_out[OUT_BYTES + GUARD_BYTES];
static uint8_t g_plane[3][PLANE_BYTES];

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

/* One 240-bit-wide, MSB-first plane of TiEmu's LCD memory (LCDMEM_W 240). */
static void plane_clear(unsigned p)
{
    memset(g_plane[p], 0, PLANE_BYTES);
}

static void plane_set(unsigned p, unsigned x, unsigned y)
{
    g_plane[p][y * 30u + (x >> 3)] |= (uint8_t)(0x80u >> (x & 7u));
}

static unsigned plane_get(unsigned p, unsigned x, unsigned y)
{
    return g_plane[p][y * 30u + (x >> 3)] & (0x80u >> (x & 7u)) ? 1u : 0u;
}

/* Wire the module's view of TiEmu to our planes. */
static void use_planes(unsigned count)
{
    unsigned i;

    for (i = 0; i < 3u; i++) {
        lcd_planebufs[i] = (i < count) ? g_plane[i] : NULL;
    }
}

static void out_poison(uint8_t value)
{
    memset(g_out, value, sizeof g_out);
}

static unsigned out_get(unsigned x, unsigned y)
{
    return g_out[(size_t)y * OUT_ROW_BYTES + x];
}

static unsigned out_row_tail_ok(unsigned x, unsigned y, uint8_t value)
{
    for (; x < OUT_ROW_BYTES; x++) {
        if (out_get(x, y) != (unsigned)value) return 0u;
    }
    return 1u;
}

static unsigned out_guard_ok(uint8_t value)
{
    size_t i;

    for (i = OUT_BYTES; i < sizeof g_out; i++) {
        if (g_out[i] != value) return 0u;
    }
    return 1u;
}

/* TiEmu's gp_seq rows, restated: ngc 1 is one plane, ngc 3 is plane 0 twice
 * then plane 1 (screen.c gp_seq[3] = {0, 0, 1}). */
static unsigned expect_4level(unsigned b0, unsigned b1)
{
    return 2u * b0 + b1;
}

static unsigned q4(unsigned raw, unsigned exposures)
{
    return (raw * 3u + exposures / 2u) / exposures;
}

static void test_sequence_table(void)
{
    const int8_t *s;

    s = t89_lcd_gray_seq(1u);
    CHECK(s && s[0] == 0 && s[1] == -1, "seq: ngc 1 is the single plane 0");
    s = t89_lcd_gray_seq(3u);
    CHECK(s && s[0] == 0 && s[1] == 0 && s[2] == 1 && s[3] == -1,
          "seq: ngc 3 is plane 0 twice then plane 1");
    s = t89_lcd_gray_seq(7u);
    CHECK(s && s[0] == 2 && s[1] == 0 && s[2] == 1 && s[3] == 0 && s[4] == 1
          && s[5] == 0 && s[6] == -1, "seq: ngc 7 is TiEmu's 6-exposure row");
    s = t89_lcd_gray_seq(8u);
    CHECK(s && s[0] == 1 && s[1] == 0 && s[2] == 2 && s[3] == 0 && s[4] == 0
          && s[5] == 1 && s[6] == 0 && s[7] == -1,
          "seq: ngc 8 is TiEmu's 7-exposure row");
    CHECK(t89_lcd_gray_seq(0u) == NULL && t89_lcd_gray_seq(2u) == NULL
          && t89_lcd_gray_seq(4u) == NULL && t89_lcd_gray_seq(5u) == NULL
          && t89_lcd_gray_seq(6u) == NULL && t89_lcd_gray_seq(9u) == NULL,
          "seq: ngc rows TiEmu marks unused have no sequence");
}

static void test_four_levels_from_plane_patterns(void)
{
    const uint8_t *planes[3] = { g_plane[0], g_plane[1], NULL };
    const int8_t *seq = t89_lcd_gray_seq(3u);
    unsigned x, y, seen = 0, bad = 0;

    /* A 2x2 block: paper-only, plane-0, plane-1, both. */
    plane_clear(0u);
    plane_clear(1u);
    plane_set(0u, 0u, 0u);
    plane_set(1u, 0u, 0u);
    plane_set(1u, 1u, 0u);
    plane_set(0u, 0u, 1u);

    out_poison(0xffu);
    t89_lcd_gray_compose(g_out, OUT_ROW_BYTES, planes, seq,
                         T89_LCD_WIDTH, T89_LCD_HEIGHT);

    CHECK(out_get(0u, 0u) == 3u, "4lvl: both planes set is ink (3)");
    CHECK(out_get(1u, 0u) == 1u, "4lvl: plane 1 alone is level 1");
    CHECK(out_get(0u, 1u) == 2u, "4lvl: plane 0 alone is level 2");
    CHECK(out_get(1u, 1u) == 0u, "4lvl: neither plane is paper (0)");

    for (y = 0u; y < T89_LCD_HEIGHT; y++) {
        for (x = 0u; x < T89_LCD_WIDTH; x++) {
            unsigned want = x < 2u && y < 2u
                ? expect_4level(plane_get(0u, x, y), plane_get(1u, x, y))
                : 0u;
            if (out_get(x, y) != want) bad++;
        }
    }
    CHECK(bad == 0u, "4lvl: the whole image matches 2*b0+b1 (%u wrong)", bad);

    for (y = 0u; y < T89_LCD_HEIGHT; y++) {
        for (x = 0u; x < T89_LCD_WIDTH; x++) {
            seen |= 1u << out_get(x, y);
        }
    }
    CHECK(seen == 0xfu, "4lvl: all four levels appear (mask %u)", seen);

    /* The model's b&w fallback: one plane, so two of the four levels. */
    plane_clear(0u);
    plane_clear(1u);
    plane_set(0u, 3u, 4u);
    seen = 0u;
    t89_lcd_gray_compose(g_out, OUT_ROW_BYTES, planes, t89_lcd_gray_seq(1u),
                         T89_LCD_WIDTH, T89_LCD_HEIGHT);
    for (y = 0u; y < T89_LCD_HEIGHT; y++) {
        for (x = 0u; x < T89_LCD_WIDTH; x++) {
            seen |= 1u << out_get(x, y);
        }
    }
    CHECK(out_get(3u, 4u) == 3u, "4lvl: one plane, a set bit is ink");
    CHECK(seen == 0x9u, "4lvl: one plane yields paper and ink only (%u)", seen);
}

/* TiEmu's gp_seq[7] = {2,0,1,0,1,0} and gp_seq[8] = {1,0,2,0,0,1,0} restated
 * as exposures per plane: the model's level is their sum (6 and 7 here). */
static unsigned raw7(unsigned b0, unsigned b1, unsigned b2)
{
    return 3u * b0 + 2u * b1 + 1u * b2;
}

static unsigned raw8(unsigned b0, unsigned b1, unsigned b2)
{
    return 4u * b0 + 2u * b1 + 1u * b2;
}

static void test_ngc7_ngc8_quantize(void)
{
    const uint8_t *planes[3];
    unsigned mask7 = 0u, mask8 = 0u, bad7 = 0u, bad8 = 0u, x, y;

    planes[0] = g_plane[0];
    planes[1] = g_plane[1];
    planes[2] = g_plane[2];
    plane_clear(0u);
    plane_clear(1u);
    plane_clear(2u);
    /* A pattern that exercises every plane in every combination. */
    for (y = 0u; y < T89_LCD_HEIGHT; y++) {
        for (x = 0u; x < T89_LCD_WIDTH; x++) {
            if (x & 1u) plane_set(0u, x, y);
            if (y & 1u) plane_set(1u, x, y);
            if (((x ^ y) >> 1) & 1u) plane_set(2u, x, y);
        }
    }

    t89_lcd_gray_compose(g_out, OUT_ROW_BYTES, planes, t89_lcd_gray_seq(7u),
                         T89_LCD_WIDTH, T89_LCD_HEIGHT);
    for (y = 0u; y < T89_LCD_HEIGHT; y++) {
        for (x = 0u; x < T89_LCD_WIDTH; x++) {
            unsigned raw = raw7(plane_get(0u, x, y), plane_get(1u, x, y),
                                plane_get(2u, x, y));
            mask7 |= 1u << out_get(x, y);
            if (out_get(x, y) != q4(raw, 6u)) bad7++;
        }
    }
    CHECK(bad7 == 0u, "q7: the 7-level model maps onto 4 exactly (%u wrong)", bad7);
    CHECK(mask7 == 0xfu, "q7: the model's 7 levels land on 0,1,2,3 (%u)", mask7);

    t89_lcd_gray_compose(g_out, OUT_ROW_BYTES, planes, t89_lcd_gray_seq(8u),
                         T89_LCD_WIDTH, T89_LCD_HEIGHT);
    for (y = 0u; y < T89_LCD_HEIGHT; y++) {
        for (x = 0u; x < T89_LCD_WIDTH; x++) {
            unsigned raw = raw8(plane_get(0u, x, y), plane_get(1u, x, y),
                                plane_get(2u, x, y));
            mask8 |= 1u << out_get(x, y);
            if (out_get(x, y) != q4(raw, 7u)) bad8++;
        }
    }
    CHECK(bad8 == 0u, "q8: the 8-level model maps onto 4 exactly (%u wrong)", bad8);
    CHECK(mask8 == 0xfu, "q8: the model's 8 levels land on 0,1,2,3 (%u)", mask8);
}

static void test_blank_is_paper(void)
{
    unsigned x, y, bad = 0;

    plane_clear(0u);
    plane_clear(1u);
    plane_clear(2u);
    use_planes(3u);
    ngc = 3;

    out_poison(0xa5u);
    t89_lcd_render(g_out);
    for (y = 0u; y < T89_LCD_HEIGHT; y++) {
        for (x = 0u; x < T89_LCD_WIDTH; x++) {
            if (out_get(x, y) != 0u) bad++;
        }
    }
    CHECK(bad == 0u, "blank: a blank LCD is all paper (%u wrong)", bad);
    CHECK(out_row_tail_ok(T89_LCD_WIDTH, 0u, 0xa5u)
          && out_row_tail_ok(T89_LCD_WIDTH, 42u, 0xa5u)
          && out_row_tail_ok(T89_LCD_WIDTH, 99u, 0xa5u),
          "blank: bytes past x=160 in a row are untouched");

    /* The app seeds ngc = 1 after a reset (gui/calc/calc.c), and then only
     * lcd_planebufs[0] is filled: that must still be paper, not a crash. */
    ngc = 1;
    lcd_planebufs[1] = NULL;
    lcd_planebufs[2] = NULL;
    out_poison(0xa5u);
    t89_lcd_render(g_out);
    CHECK(out_get(0u, 0u) == 0u && out_get(159u, 99u) == 0u,
          "blank: ngc 1 with only plane 0 is paper");
}

static void test_pixel_placement(void)
{
    unsigned x, y, bad = 0;

    plane_clear(0u);
    plane_clear(1u);
    plane_set(0u, 5u, 7u);
    plane_set(1u, 5u, 7u);
    plane_set(0u, 159u, 99u);
    plane_set(1u, 0u, 0u);
    use_planes(2u);
    ngc = 3;

    out_poison(0x5au);
    t89_lcd_render(g_out);

    CHECK(out_get(5u, 7u) == 3u, "place: (5,7) is at byte 256*7+5 = 3");
    CHECK(g_out[(size_t)7u * OUT_ROW_BYTES + 5u] == 3u,
          "place: (5,7) really is byte 1797");
    CHECK(out_get(159u, 99u) == 2u, "place: (159,99) is plane 0 alone");
    CHECK(out_get(0u, 0u) == 1u, "place: (0,0) is plane 1 alone");
    CHECK(out_get(4u, 7u) == 0u && out_get(6u, 7u) == 0u
          && out_get(5u, 6u) == 0u && out_get(5u, 8u) == 0u,
          "place: the neighbours of (5,7) stay paper");

    for (y = 0u; y < T89_LCD_HEIGHT; y++) {
        for (x = 0u; x < T89_LCD_WIDTH; x++) {
            unsigned want =
                expect_4level(plane_get(0u, x, y), plane_get(1u, x, y));
            if (out_get(x, y) != want) bad++;
        }
    }
    CHECK(bad == 0u, "place: every pixel lands at 256*y+x (%u wrong)", bad);
    bad = 0u;
    for (y = 0u; y < T89_LCD_HEIGHT; y++) {
        if (!out_row_tail_ok(T89_LCD_WIDTH, y, 0x5au)) bad++;
    }
    CHECK(bad == 0u, "place: all 100 row tails are untouched (%u bad)", bad);
}

static void test_buffer_extent(void)
{
    unsigned x, y, meaningful = 0, bad = 0;

    plane_clear(0u);
    plane_clear(1u);
    for (y = 0u; y < 100u; y++) {
        for (x = 0u; x < 160u; x++) {
            if ((x + y) & 1u) plane_set(0u, x, y);
            if ((x / 3u + y / 5u) & 1u) plane_set(1u, x, y);
        }
    }
    use_planes(2u);
    ngc = 3;

    out_poison(0x3cu);
    t89_lcd_render(g_out);

    for (y = 0u; y < T89_LCD_HEIGHT; y++) {
        for (x = 0u; x < T89_LCD_WIDTH; x++) {
            if (out_get(x, y) <= 3u) meaningful++;
        }
    }
    CHECK(meaningful == T89_LCD_WIDTH * T89_LCD_HEIGHT,
          "extent: exactly 100 rows of 160 meaningful bytes (%u)", meaningful);
    CHECK(out_guard_ok(0x3cu),
          "extent: nothing is written at or past byte 256*100");
    CHECK(g_out[(size_t)99u * OUT_ROW_BYTES + 159u] <= 3u,
          "extent: the last meaningful byte is 256*99+159 = 25503");
    CHECK(g_out[OUT_BYTES - 1u] == 0x3cu && g_out[(size_t)99u * OUT_ROW_BYTES + 160u] == 0x3cu,
          "extent: the last row's 96 tail bytes are untouched");
    for (y = 0u; y < T89_LCD_HEIGHT; y++) {
        for (x = 0u; x < T89_LCD_WIDTH; x++) {
            unsigned want = expect_4level(plane_get(0u, x, y), plane_get(1u, x, y));
            if (out_get(x, y) != want) bad++;
        }
    }
    CHECK(bad == 0u, "extent: the pattern survives end to end (%u wrong)", bad);

    /* A stride that is not the SDRAM's: only width bytes a row are touched. */
    {
        uint8_t narrow[16];
        const uint8_t *planes[3];
        unsigned i;

        memset(narrow, 0x77, sizeof narrow);
        planes[0] = g_plane[0];
        planes[1] = g_plane[1];
        planes[2] = NULL;
        t89_lcd_gray_compose(narrow, 8u, planes, t89_lcd_gray_seq(3u), 3u, 2u);
        for (i = 0u; i < 2u; i++) {
            CHECK(narrow[i * 8u + 3u] == 0x77u && narrow[i * 8u + 7u] == 0x77u,
                  "extent: compose leaves byte %u past the row's width", i * 8u + 3u);
        }
        CHECK(narrow[0] <= 3u && narrow[8] <= 3u,
              "extent: compose writes the row's first 3 bytes");
    }
}

static void test_render_missing_planes(void)
{
    plane_clear(0u);
    plane_set(0u, 2u, 2u);
    plane_clear(1u);

    /* ngc says two planes but gscales.c has not filled the second one in:
     * the model must read that as paper, not fault. */
    lcd_planebufs[0] = g_plane[0];
    lcd_planebufs[1] = NULL;
    lcd_planebufs[2] = NULL;
    ngc = 3;

    out_poison(0u);
    t89_lcd_render(g_out);
    CHECK(out_get(2u, 2u) == 2u, "null: a missing plane 1 leaves plane 0 alone");
    CHECK(out_get(3u, 2u) == 0u, "null: a missing plane never sets a pixel");

    /* An ngc the model has no sequence for falls back to TiEmu's b&w row. */
    ngc = 2;
    t89_lcd_render(g_out);
    CHECK(out_get(2u, 2u) == 3u && out_get(3u, 2u) == 0u,
          "null: an ngc with no sequence falls back to one plane");
}

static void test_mode_string(void)
{
    const char *a, *b, *c;

    ngc = 1;
    a = t89_lcd_mode();
    b = t89_lcd_mode();
    CHECK(strcmp(a, "1bit") == 0, "mode: one plane is 1bit (got %s)", a);
    CHECK(a == b, "mode: the string is stable across calls");

    ngc = 3;
    c = t89_lcd_mode();
    CHECK(strcmp(c, "gray") == 0, "mode: a composed plane model is gray (got %s)", c);
    CHECK(c != b, "mode: the two modes are distinct strings");

    ngc = 3;
    out_poison(0u);
    t89_lcd_render(g_out);
    CHECK(t89_lcd_mode() == c, "mode: a render does not change the mode");
    CHECK(strcmp(t89_lcd_mode(), "gray") == 0 && strcmp(t89_lcd_mode(), "gray") == 0,
          "mode: still gray after rendering");

    ngc = 0;
    CHECK(strcmp(t89_lcd_mode(), "1bit") == 0,
          "mode: an unseeded ngc reads as 1bit");
    CHECK(t89_lcd_gray_mode(7u) == c, "mode: ngc 7 shares the gray string");
}

int main(void)
{
    test_sequence_table();
    test_four_levels_from_plane_patterns();
    test_ngc7_ngc8_quantize();
    test_blank_is_paper();
    test_pixel_placement();
    test_buffer_extent();
    test_render_missing_planes();
    test_mode_string();

    printf("t89_lcd: %d checks passed\n", g_checks - g_fails);
    return (g_fails == 0 && g_checks >= 30) ? 0 : 1;
}
