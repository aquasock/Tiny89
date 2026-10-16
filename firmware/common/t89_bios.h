/* SPDX-License-Identifier: MIT
 *
 * The BIOS: what the user provides, turned into what the emulator boots.
 */
#ifndef T89_BIOS_H
#define T89_BIOS_H

#include <stddef.h>
#include <stdint.h>

/* The flash image the calculator's emulator boots: 4 MiB, which is the size
 * TiEmu's calculator type expects and the size the fabric was built around. */
#define T89_BIOS_BYTES (4u * 1024u * 1024u)

/* The largest upgrade file taken.  The one TI ships for the Titanium is
 * 1,400,021 bytes, so this has room to spare without being a licence to
 * allocate anything. */
#define T89_BIOS_MAX (2u * 1024u * 1024u)

/* Turn the upgrade file the user provides -- the .89u that TI distributes,
 * which is a "**TIFL**" container of records -- into the 4 MiB flash image,
 * writing every byte of `flash`.  Returns 0, or -1 with a short reason in
 * `why` for a file that is not one this project can use.
 *
 * The arithmetic and the checks are tools/convert_89u.py's, which in turn
 * follows TiEmu 3.04's ti68k_convert_tib_to_image; tests/run_t89_bios_test.sh
 * runs both on the real file and compares them byte for byte. */
int t89_bios_from_89u(const uint8_t *source, size_t length, uint8_t *flash,
                      char *why, size_t why_len);

#endif /* T89_BIOS_H */
