/* SPDX-License-Identifier: MIT */
#include "t89_save.h"

uint32_t t89_save_crc32(uint32_t crc, const uint8_t *data, size_t length)
{
    size_t i;

    crc = ~crc;
    for (i = 0; i < length; i++) {
        int bit;

        crc ^= data[i];
        for (bit = 0; bit < 8; bit++) {
            crc = (crc >> 1) ^ (UINT32_C(0xedb88320) & (0u - (crc & 1u)));
        }
    }
    return ~crc;
}

void t89_save_hex(char *out, const uint8_t *bytes, size_t n)
{
    static const char digits[] = "0123456789abcdef";
    size_t i;

    for (i = 0; i < n; i++) {
        out[2u * i] = digits[bytes[i] >> 4];
        out[2u * i + 1u] = digits[bytes[i] & 15u];
    }
    out[2u * n] = '\0';
}

int t89_save_is_blank(const uint8_t *data, size_t length)
{
    size_t i;

    for (i = 1; i < length; i++) {
        if (data[i] != data[0]) return 0;
    }
    return 1;
}
