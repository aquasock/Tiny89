/* SPDX-License-Identifier: MIT
 *
 * The BIOS: a TI-89 Titanium .89u upgrade, turned into the 4 MiB flash image
 * the calculator's emulator boots.
 *
 * What the user is asked for is the upgrade TI ships -- a "**TIFL**"
 * container of records whose last one holds the operating system -- and not a
 * raw image, because that is the file a person has and it is a third of the
 * size (1,400,021 bytes against 4,194,304), so a launch moves less of it over
 * the link.  The conversion therefore happens here rather than on a PC: the
 * card is where the file lives and the board has no computer beside it.
 *
 * This is tools/convert_89u.py in C, and deliberately the same arithmetic and
 * the same checks, because that tool's output is what earlier cycles booted:
 * TiEmu 3.04's ti68k_convert_tib_to_image layout -- the boot sector at the
 * front of the flash, a "TiEmu made this image" marker, the 24-byte hardware
 * parameter block its synthetic image carries, and the system part at
 * 0x12000.  tests/run_t89_bios_test.sh runs both on the real file and
 * compares them byte for byte, so the two cannot drift apart unnoticed.
 */
#include <stdint.h>
#include <string.h>

#include "t89_bios.h"

#define TIFL_SIGNATURE   "**TIFL**"
#define TIFL_HEADER_SIZE 78u

/* TiEmu's device and data types for a TI-89 Titanium OS record, and the
 * hardware generation its synthetic image declares. */
#define TI89_DEVICE_TYPE  0x98u
#define TI89_OS_DATA_TYPE 0x23u
#define TI89T_HARDWARE_ID 9u
#define TI89T_HW_TYPE     3u

/* Where the pieces sit in the flash image: the boot sector the upgrade
 * carries, the marker and parameter block TiEmu writes for itself, and the
 * system part. */
#define BOOT_OFFSET              0x88u
#define BOOT_SIZE                256u
#define TIEMU_MARKER_OFFSET      0x100u
#define PARAMETER_POINTER_OFFSET 0x104u
#define PARAMETER_BLOCK_OFFSET   0x108u
#define PARAMETER_BLOCK_LENGTH   24u
#define SYSTEM_PART_OFFSET       0x12000u

static uint32_t le32(const uint8_t *p)
{
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) |
           ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

static void put_be16(uint8_t *p, uint16_t value)
{
    p[0] = (uint8_t)(value >> 8);
    p[1] = (uint8_t)value;
}

static void put_be32(uint8_t *p, uint32_t value)
{
    p[0] = (uint8_t)(value >> 24);
    p[1] = (uint8_t)(value >> 16);
    p[2] = (uint8_t)(value >> 8);
    p[3] = (uint8_t)value;
}

static void refuse(char *why, size_t why_len, const char *text)
{
    if (why != NULL && why_len != 0) {
        strncpy(why, text, why_len - 1);
        why[why_len - 1] = '\0';
    }
}

int t89_bios_from_89u(const uint8_t *source, size_t length, uint8_t *flash,
                      char *why, size_t why_len)
{
    size_t offset = 0;
    size_t record = 0, record_length = 0;
    uint8_t device_type = 0, data_type = 0;
    uint8_t rom_base;
    int any = 0;

    if (source == NULL || flash == NULL) {
        refuse(why, why_len, "no image");
        return -1;
    }

    /* Walk the container to its LAST record, which is the one the Python tool
     * takes, and remember what that record says about itself. */
    while (offset < length) {
        size_t start;
        uint32_t payload;

        if (length - offset < TIFL_HEADER_SIZE) {
            refuse(why, why_len, "a record header runs past the end");
            return -1;
        }
        if (memcmp(source + offset, TIFL_SIGNATURE, sizeof(TIFL_SIGNATURE) - 1) != 0) {
            refuse(why, why_len, "not a **TIFL** upgrade");
            return -1;
        }
        payload = le32(source + offset + 74);
        start = offset + TIFL_HEADER_SIZE;
        if ((size_t)payload > length - start) {
            refuse(why, why_len, "a record claims more than the file holds");
            return -1;
        }
        record = start;
        record_length = payload;
        device_type = source[offset + 48];
        data_type = source[offset + 49];
        any = 1;
        offset = start + payload;
    }

    if (!any) {
        refuse(why, why_len, "not a **TIFL** upgrade");
        return -1;
    }
    if (record_length < BOOT_OFFSET + BOOT_SIZE) {
        refuse(why, why_len, "the OS record is too short");
        return -1;
    }
    if (record_length > T89_BIOS_BYTES - SYSTEM_PART_OFFSET) {
        refuse(why, why_len, "the OS does not fit in 4 MiB");
        return -1;
    }
    if (device_type != TI89_DEVICE_TYPE) {
        refuse(why, why_len, "not a TI-89 image");
        return -1;
    }
    if (data_type != TI89_OS_DATA_TYPE) {
        refuse(why, why_len, "the last record is not an OS");
        return -1;
    }

    /* The upgrade's boot vector says which ROM base it is built for, and only
     * the Titanium's is accepted. */
    rom_base = (uint8_t)(source[record + BOOT_OFFSET + 5] & 0xF0u);
    if (rom_base != 0x80u) {
        refuse(why, why_len, "not a TI-89 Titanium image");
        return -1;
    }

    memset(flash, 0xFF, T89_BIOS_BYTES);

    memcpy(flash, source + record + BOOT_OFFSET, BOOT_SIZE);

    /* The marker, the pointer TiEmu follows, and the parameter block it
     * synthesises: length 24, hardware id 9, revision 2, boots 1 and 2, and
     * the hardware generation. */
    flash[TIEMU_MARKER_OFFSET + 0] = 0xFEu;
    flash[TIEMU_MARKER_OFFSET + 1] = 0xEDu;
    flash[TIEMU_MARKER_OFFSET + 2] = 0xBAu;
    flash[TIEMU_MARKER_OFFSET + 3] = 0xBEu;
    put_be32(flash + PARAMETER_POINTER_OFFSET,
             ((uint32_t)rom_base << 16) | PARAMETER_BLOCK_OFFSET);
    put_be16(flash + PARAMETER_BLOCK_OFFSET + 0, PARAMETER_BLOCK_LENGTH);
    put_be32(flash + PARAMETER_BLOCK_OFFSET + 2, TI89T_HARDWARE_ID);
    put_be32(flash + PARAMETER_BLOCK_OFFSET + 6, 2u);
    put_be32(flash + PARAMETER_BLOCK_OFFSET + 10, 1u);
    put_be32(flash + PARAMETER_BLOCK_OFFSET + 14, 2u);
    put_be32(flash + PARAMETER_BLOCK_OFFSET + 18, 1u);
    put_be32(flash + PARAMETER_BLOCK_OFFSET + 22, TI89T_HW_TYPE);

    memcpy(flash + SYSTEM_PART_OFFSET, source + record, record_length);

    if (why != NULL && why_len != 0) {
        why[0] = '\0';
    }
    return 0;
}
