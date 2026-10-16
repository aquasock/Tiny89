/* SPDX-License-Identifier: MIT
 *
 * The BIOS conversion's host test.
 *
 *   tests/test_t89_bios.c --checks                     the refusals, in memory
 *   tests/test_t89_bios.c <upgrade.89u> <out.raw>      convert a real file
 *
 * The second form exists so tests/run_t89_bios_test.sh can hand the same file
 * to tools/convert_89u.py and compare the two outputs byte for byte: the
 * Python tool is what earlier cycles booted, so the C here has to agree with
 * it exactly rather than merely look reasonable.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "t89_bios.h"

static int g_fails;

static void check(int ok, const char *name)
{
    if (ok) {
        printf("  ok   %s\n", name);
    } else {
        printf("  FAIL %s\n", name);
        g_fails++;
    }
}

/* A "**TIFL**" record header, as far as this file cares about one. */
static void header(uint8_t *h, uint32_t payload, uint8_t device, uint8_t type)
{
    memset(h, 0, 78);
    memcpy(h, "**TIFL**", 8);
    h[48] = device;
    h[49] = type;
    h[74] = (uint8_t)payload;
    h[75] = (uint8_t)(payload >> 8);
    h[76] = (uint8_t)(payload >> 16);
    h[77] = (uint8_t)(payload >> 24);
}

static int refusals(void)
{
    static uint8_t flash[T89_BIOS_BYTES];
    uint8_t file[4096];
    char why[64];

    printf("t89_bios refusals:\n");

    memset(file, 0x41, sizeof file);
    check(t89_bios_from_89u(file, sizeof file, flash, why, sizeof why) != 0,
          "garbage is refused");
    check(strstr(why, "TIFL") != NULL, "and it says what it wanted");

    /* A header that claims more than the file holds. */
    memset(file, 0, sizeof file);
    header(file, 100000, 0x98, 0x23);
    check(t89_bios_from_89u(file, sizeof file, flash, why, sizeof why) != 0,
          "a record longer than the file is refused");

    /* A well-formed container whose payload is too short to hold a boot
     * sector, and one whose device type is not the Titanium's. */
    memset(file, 0, sizeof file);
    header(file, 0x100, 0x98, 0x23);
    check(t89_bios_from_89u(file, 78 + 0x100, flash, why, sizeof why) != 0,
          "an OS record too short for a boot sector is refused");

    memset(file, 0, sizeof file);
    header(file, 0x1000, 0x00, 0x23);
    check(t89_bios_from_89u(file, 78 + 0x1000, flash, why, sizeof why) != 0,
          "a record that is not a TI-89 image is refused");

    memset(file, 0, sizeof file);
    header(file, 0x1000, 0x98, 0x00);
    check(t89_bios_from_89u(file, 78 + 0x1000, flash, why, sizeof why) != 0,
          "a last record that is not an OS is refused");

    return g_fails;
}

int main(int argc, char **argv)
{
    FILE *in, *out;
    uint8_t *source, *flash;
    long size;
    char why[64];

    if (refusals() != 0) {
        printf("t89_bios: FAIL (refusals)\n");
        return 1;
    }

    if (argc == 2 && strcmp(argv[1], "--checks") == 0) {
        printf("t89_bios: PASS (refusals)\n");
        return 0;
    }
    if (argc != 3) {
        fprintf(stderr, "usage: %s <upgrade.89u> <out.raw> | --checks\n", argv[0]);
        return 2;
    }

    in = fopen(argv[1], "rb");
    if (in == NULL) {
        perror(argv[1]);
        return 2;
    }
    if (fseek(in, 0, SEEK_END) != 0 || (size = ftell(in)) <= 0 ||
        fseek(in, 0, SEEK_SET) != 0) {
        fprintf(stderr, "%s: cannot size\n", argv[1]);
        fclose(in);
        return 2;
    }
    source = malloc((size_t)size);
    flash = malloc(T89_BIOS_BYTES);
    if (source == NULL || flash == NULL) {
        fprintf(stderr, "out of memory\n");
        fclose(in);
        return 2;
    }
    if (fread(source, 1, (size_t)size, in) != (size_t)size) {
        fprintf(stderr, "%s: short read\n", argv[1]);
        fclose(in);
        return 2;
    }
    fclose(in);

    if (t89_bios_from_89u(source, (size_t)size, flash, why, sizeof why) != 0) {
        printf("  FAIL conversion: %s\n", why);
        printf("t89_bios: FAIL\n");
        return 1;
    }

    out = fopen(argv[2], "wb");
    if (out == NULL) {
        perror(argv[2]);
        return 2;
    }
    if (fwrite(flash, 1, T89_BIOS_BYTES, out) != T89_BIOS_BYTES) {
        fprintf(stderr, "%s: short write\n", argv[2]);
        fclose(out);
        return 2;
    }
    fclose(out);

    printf("converted %s (%ld bytes) into %u bytes\n", argv[1], size,
           (unsigned)T89_BIOS_BYTES);
    return 0;
}
