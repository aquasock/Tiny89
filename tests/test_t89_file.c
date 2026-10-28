/*
 * Host test for the two container shapes a TI-89 file comes in.
 *
 * Both buffers here are built by the test, so nothing private is needed and
 * the layout is stated in the test rather than implied by a fixture:
 *
 *   regular   8-byte signature, data offset 0x52 at 60, name at 64, type at
 *             72, attribute at 73, whole-file length at 76, the 0x5AA5 marker
 *             at 80, payload at 82, and a two-byte sum of the payload at the
 *             end.
 *   **TIFL**  78-byte header with the name at 17 and the type at 49, the
 *             payload's length at 74, the payload at 78, and the record ending
 *             there -- the file is exactly 78 + length, with no sum after it.
 *
 * The TIFL case is the one that was wrong: the reader used to expect two more
 * bytes and a sum, which no real Flash App has, so every .89k in the user's set
 * failed to parse (core-log entry 22).  The operating system's own upgrade file
 * has the same layout -- 1,400,021 bytes of a 78-byte header and a 1,399,943-byte
 * payload -- and t89_bios.c has relied on that since entry 8.
 *
 *   tests/run_t89_file_test.sh
 */
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "t89_file.h"

static int checks;
static int fails;

static void check(int ok, const char *what)
{
    checks++;
    if (!ok) {
        fails++;
        printf("  FAIL: %s\n", what);
    }
}

static void put32(uint8_t *p, uint32_t v)
{
    p[0] = (uint8_t)v;
    p[1] = (uint8_t)(v >> 8);
    p[2] = (uint8_t)(v >> 16);
    p[3] = (uint8_t)(v >> 24);
}

static void put16(uint8_t *p, uint16_t v)
{
    p[0] = (uint8_t)v;
    p[1] = (uint8_t)(v >> 8);
}

static uint16_t byte_sum(const uint8_t *p, unsigned n)
{
    uint16_t sum = 0;
    while (n--) {
        sum = (uint16_t)(sum + *p++);
    }
    return sum;
}

/* A regular variable container holding `payload`, whose whole length is
 * 82 + payload + 2. */
static unsigned make_regular(uint8_t *buf, const char *name, uint8_t type,
                             const uint8_t *payload, unsigned length)
{
    const unsigned total = 82u + length + 2u;
    memset(buf, 0, total);
    memcpy(buf, "**TI89**", 8);
    put32(buf + 60, 0x52u);
    memcpy(buf + 64, name, strlen(name));
    buf[72] = type;
    buf[73] = 0x00;
    put32(buf + 76, total);
    buf[80] = 0xA5;
    buf[81] = 0x5A;
    memcpy(buf + 82, payload, length);
    put16(buf + 82 + length, byte_sum(payload, length));
    return total;
}

/* A TIFL container holding `payload`, whose whole length is 78 + payload. */
static unsigned make_tifl(uint8_t *buf, const char *name, uint8_t type,
                          const uint8_t *payload, unsigned length)
{
    const unsigned total = 78u + length;
    memset(buf, 0, total);
    memcpy(buf, "**TIFL**", 8);
    buf[16] = 0x01;
    memcpy(buf + 17, name, strlen(name));
    buf[48] = 0x98;
    buf[49] = type;
    put32(buf + 74, length);
    memcpy(buf + 78, payload, length);
    return total;
}

int main(void)
{
    static uint8_t buf[4096];
    static const uint8_t payload[] = { 0x01, 0x02, 0x03, 0x04, 0xFE, 0xFF, 0x7F };
    t89_var_t var;
    unsigned total;

    /* ---- a regular variable reads as itself ---- */
    total = make_regular(buf, "metrodat", 0x1C, payload, sizeof payload);
    check(t89_file_parse(buf, total, &var) != 0, "a regular container was refused");
    check(strcmp(var.name, "metrodat") == 0, "the regular name was not read");
    check(var.type == 0x1C, "the regular type was not read");
    check(var.size == sizeof payload, "the regular payload length was not read");
    check(var.data == buf + 82, "the regular payload does not point at 82");
    check(memcmp(var.data, payload, sizeof payload) == 0, "the regular payload differs");

    /* A program's type, to be sure the type byte is not hard-coded anywhere. */
    total = make_regular(buf, "runmetro", 0x12, payload, sizeof payload);
    check(t89_file_parse(buf, total, &var) != 0, "a program container was refused");
    check(var.type == 0x12, "the program type was not read");

    /* ---- a TIFL Flash App reads as itself, with no trailing sum ---- */
    total = make_tifl(buf, "dvMEPro", 0x24, payload, sizeof payload);
    check(t89_file_parse(buf, total, &var) != 0, "a TIFL container was refused");
    check(strcmp(var.name, "dvMEPro") == 0, "the TIFL name was not read");
    check(var.type == 0x24, "the TIFL type was not read");
    check(var.size == sizeof payload, "the TIFL payload length was not read");
    check(var.data == buf + 78, "the TIFL payload does not point at 78");

    /* The whole file is exactly the header plus the payload: one byte more is
     * fine (a longer buffer), one byte less is not. */
    check(t89_file_parse(buf, total - 1, &var) == 0,
          "a TIFL record one byte short was accepted");
    check(t89_file_parse(buf, total, &var) != 0,
          "a TIFL record of exactly 78 + length was refused");

    /* ---- and the things that must be refused still are ---- */
    total = make_regular(buf, "badsum", 0x1C, payload, sizeof payload);
    buf[82 + sizeof payload] ^= 0xFF;
    check(t89_file_parse(buf, total, &var) == 0, "a regular container with a bad sum was accepted");

    total = make_regular(buf, "wrongoff", 0x1C, payload, sizeof payload);
    put32(buf + 60, 0x40u);
    check(t89_file_parse(buf, total, &var) == 0, "a container with the wrong data offset was accepted");

    total = make_regular(buf, "shortlen", 0x1C, payload, sizeof payload);
    total -= 1;
    check(t89_file_parse(buf, total, &var) == 0, "a container longer than its buffer was accepted");

    total = make_tifl(buf, "zerolen", 0x24, payload, sizeof payload);
    put32(buf + 74, 0u);
    check(t89_file_parse(buf, total, &var) == 0, "a TIFL record of zero length was accepted");

    total = make_tifl(buf, "toolong", 0x24, payload, sizeof payload);
    put32(buf + 74, (uint32_t)total);
    check(t89_file_parse(buf, total, &var) == 0, "a TIFL record longer than the file was accepted");

    memset(buf, 0, 128);
    check(t89_file_parse(buf, 128, &var) == 0, "an empty buffer was accepted");
    check(t89_file_parse(NULL, 0, &var) == 0, "a null buffer was accepted");

    /* An empty name is not a variable anyone can install. */
    total = make_regular(buf, "", 0x1C, payload, sizeof payload);
    check(t89_file_parse(buf, total, &var) == 0, "a container with no name was accepted");

    printf("t89_file: %s (%d checks)\n", fails == 0 ? "PASS" : "FAIL", checks);
    return fails == 0 ? 0 : 1;
}
