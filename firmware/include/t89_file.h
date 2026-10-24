/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef TINY89_FILE_H
#define TINY89_FILE_H

#include <stdint.h>

/* A parsed calculator file: what the link sender needs and nothing else. */
typedef struct {
    char name[9];            /* the variable's name, 8 characters at most */
    uint8_t type;            /* TI variable type: 0x24 app, 0x1C zip, 0x12 prgm */
    const uint8_t *data;     /* points into the caller's buffer */
    unsigned size;
} t89_var_t;

/* Parse a file's bytes.  Returns non-zero when the container was understood
 * and the payload's own checksum agreed; `out->data` then points into `buf`. */
int t89_file_parse(const uint8_t *buf, unsigned len, t89_var_t *out);

#endif
