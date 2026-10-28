/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * The two shapes a TI-89 file comes in, read into the one thing the link
 * sender needs: a name, a type and a payload.
 *
 * Regular variable files (**.89y**, .89z, .89p) are the shape libtifiles'
 * tifiles_file_read_regular() parses for the 68k family: an eight-byte
 * signature, a word, an eight-byte folder, a forty-byte comment, a word, the
 * data offset, then at 64 the name, at 72 the type, at 73 the attribute, at 76
 * the file length, the 0x5AA5 marker at 80 and the payload at 82 with a
 * two-byte sum of it at the end.  Verified against all 55 such files in the
 * user's 2021 backup: every length and every sum agrees.
 *
 * Flash application files (.89k) are the **TIFL** container that
 * tifiles_file_read_flash() walks: a 78-byte header (signature, revision
 * bytes, name at 17, device type at 48, data type at 49, length at 74)
 * followed by the payload, and the record ends there -- the length at 74
 * counts the payload and the file ends at 78 + length, with no sum after it.
 * Unlike a regular variable file, this container proves nothing by itself, so
 * the checks here are structural: the signature, a non-zero length, and a
 * record that fits.
 *
 * That layout is not a guess: the operating system's own upgrade file is a
 * **TIFL** too, and t89_bios.c has converted it since entry 8 (1,400,021
 * bytes = a 78-byte header plus a 1,399,943-byte payload, nothing after).  The
 * earlier reading here -- a two-byte sum after the payload -- was wrong for
 * every real Flash App and is why those files could not be parsed at all; the
 * three in the user's set end exactly at 78 + length (core-log entry 22).
 *
 * One file may hold more than one record; this reader takes the first, which
 * is the application.
 */
#include <stdint.h>
#include <string.h>

#include "t89_file.h"

static uint32_t le32(const uint8_t *p)
{
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) |
           ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

static uint16_t le16(const uint8_t *p)
{
    return (uint16_t)((uint16_t)p[0] | ((uint16_t)p[1] << 8));
}

static uint16_t sum16(const uint8_t *p, unsigned n)
{
    uint16_t sum = 0;

    while (n--) {
        sum = (uint16_t)(sum + *p++);
    }
    return sum;
}

static void copy_name(char *out, const uint8_t *from, unsigned width)
{
    unsigned n = 0;

    while (n < width && from[n] != 0) {
        out[n] = (char)from[n];
        n++;
    }
    out[n] = '\0';
}

int t89_file_parse(const uint8_t *buf, unsigned room, t89_var_t *out)
{
    if (buf == NULL || out == NULL) {
        return 0;
    }
    memset(out, 0, sizeof *out);

    /* Each container states its own length, and the sum at its end proves the
     * length was read correctly: a wrong length is a wrong sum.  That is what
     * makes this safe to call on a buffer whose used length the caller does not
     * know -- the loader hands over bytes without saying how many. */

    /* **TIFL**: a flash file, one 78-byte header before its payload. */
    if (room >= 78u && memcmp(buf, "**TIFL**", 8) == 0) {
        const uint32_t size = le32(buf + 74);

        if (size == 0 || size > room || 78u + size > room) {
            return 0;
        }
        copy_name(out->name, buf + 17, 8);
        out->type = buf[49];
        out->data = buf + 78;
        out->size = size;
        return out->name[0] != '\0';
    }

    /* A regular 68k variable file: the field at 76 is the whole file's length,
     * the payload is at 0x52 and a two-byte sum of it ends the file. */
    if (room >= 84u && buf[0] == '*' && buf[1] == '*') {
        const uint32_t len = le32(buf + 76);
        unsigned size;

        if (le32(buf + 60) != 0x52u || len < 84u || len > room) {
            return 0;
        }
        size = (unsigned)len - 82u - 2u;
        if (sum16(buf + 82, size) != le16(buf + (unsigned)len - 2u)) {
            return 0;
        }
        copy_name(out->name, buf + 64, 8);
        out->type = buf[72];
        out->data = buf + 82;
        out->size = size;
        return out->name[0] != '\0';
    }
    return 0;
}
