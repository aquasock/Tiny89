/* SPDX-License-Identifier: MIT */
/*
 * The RAM snapshot's host test: the CRC-32 the board checks a save with, the
 * hex a line goes out as, and the blank test that tells an unwritten save from
 * a real one.  No board and no RISC-V toolchain; see tests/run_t89_save_test.sh.
 *
 * The CRC fixtures are zlib's, and zlib's is the same algorithm ae350run and
 * this module implement: CRC-32/ISO-HDLC, reflected, initialised to 0, final
 * XOR -- the one whose check value for "123456789" is 0xCBF43926.
 */
#include <stdio.h>
#include <string.h>

#include "t89_save.h"

static int g_checks, g_fails;

static void check(int cond, const char *name)
{
    g_checks++;
    if (!cond) {
        g_fails++;
        printf("FAIL %s\n", name);
    }
}

int main(void)
{
    static uint8_t ram[T89_SAVE_BYTES];
    char line[T89_SAVE_LINE_CHARS + 1];
    uint8_t bytes[8] = { 0x00, 0x0f, 0x10, 0xa5, 0xff, 0x7e, 0x01, 0x80 };
    static const char text[] = "The quick brown fox jumps over the lazy dog";
    uint32_t x = 12345;
    uint32_t chunked;
    int i;

    /* The CRC the board will judge the save by. */
    check(t89_save_crc32(0, NULL, 0) == 0x00000000u, "crc: nothing is zero");
    check(t89_save_crc32(0, (const uint8_t *)"123456789", 9) == 0xcbf43926u,
          "crc: the standard check value");
    check(t89_save_crc32(0, (const uint8_t *)text, sizeof text - 1) == 0x414fa339u,
          "crc: a known sentence");

    /* ae350run accumulates the CRC a block at a time as the lines arrive, so
     * continuing a CRC must agree with one pass over the whole thing. */
    chunked = t89_save_crc32(0, (const uint8_t *)text, 10);
    chunked = t89_save_crc32(chunked, (const uint8_t *)text + 10,
                             sizeof text - 1 - 10);
    check(chunked == 0x414fa339u, "crc: two chunks equal one pass");
    chunked = t89_save_crc32(0, (const uint8_t *)text, 1);
    for (i = 1; i < (int)sizeof text - 1; i++) {
        chunked = t89_save_crc32(chunked, (const uint8_t *)text + i, 1);
    }
    check(chunked == 0x414fa339u, "crc: and a byte at a time");

    /* A kilobyte with no structure, so a wrong polynomial or bit order shows. */
    memset(ram, 0, sizeof ram);
    for (i = 0; i < 1000; i++) {
        x = (x * 1103515245u + 12345u) & 0x7fffffffu;
        ram[i] = (uint8_t)((x >> 16) & 0xffu);
    }
    check(ram[0] == 220 && ram[7] == 90, "pattern: the fixture's generator");
    check(t89_save_crc32(0, ram, 1000) == 0xfcfd9f35u,
          "crc: a kilobyte of that pattern");

    /* The hex a line goes out as. */
    t89_save_hex(line, bytes, sizeof bytes);
    check(strcmp(line, "000f10a5ff7e0180") == 0, "hex: eight bytes, sixteen digits");
    check(strlen(line) == 2 * sizeof bytes, "hex: two digits a byte");
    memset(ram, 0xff, T89_SAVE_LINE_BYTES);
    t89_save_hex(line, ram, T89_SAVE_LINE_BYTES);
    check(strlen(line) == T89_SAVE_LINE_CHARS, "hex: a full line is 64 digits");
    check(strspn(line, "0123456789abcdef") == T89_SAVE_LINE_CHARS,
          "hex: lower case digits only");
    check(line[0] == 'f' && line[T89_SAVE_LINE_CHARS - 1] == 'f',
          "hex: and the bytes are in order");

    /* Blank: every byte the same, which is what untouched RAM looks like. */
    memset(ram, 0x00, T89_SAVE_BYTES);
    check(t89_save_is_blank(ram, T89_SAVE_BYTES) == 1, "blank: zeroed RAM");
    memset(ram, 0xff, T89_SAVE_BYTES);
    check(t89_save_is_blank(ram, T89_SAVE_BYTES) == 1, "blank: erased RAM");
    memset(ram, 0x5a, T89_SAVE_BYTES);
    check(t89_save_is_blank(ram, T89_SAVE_BYTES) == 1, "blank: any single value");
    ram[T89_SAVE_BYTES - 1] = 0x5b;
    check(t89_save_is_blank(ram, T89_SAVE_BYTES) == 0,
          "blank: a byte at the end is not");
    ram[T89_SAVE_BYTES - 1] = 0x5a;
    ram[0] = 0x00;
    check(t89_save_is_blank(ram, T89_SAVE_BYTES) == 0,
          "blank: and a byte at the front is not");
    check(t89_save_is_blank(ram, 1) == 1, "blank: one byte is trivially blank");
    check(t89_save_is_blank(ram, 0) == 1, "blank: and nothing at all");

    /* The sizes the protocol works in: the calculator's RAM is a whole number
     * of TinyNES's lines, which is why the save has no ragged last line. */
    check(T89_SAVE_BYTES % T89_SAVE_LINE_BYTES == 0,
          "size: RAM is a whole number of lines");
    check(T89_SAVE_BYTES / T89_SAVE_LINE_BYTES == 8192,
          "size: 8192 lines of 32 bytes");

    if (g_fails == 0)
        printf("t89_save: %d checks passed\n", g_checks);
    else
        printf("t89_save: %d FAILURES among %d checks\n", g_fails, g_checks);
    return (g_fails == 0) ? 0 : 1;
}
