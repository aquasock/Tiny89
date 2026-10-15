/* SPDX-License-Identifier: GPL-2.0-or-later */

#include <errno.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "hw.h"
#include "images.h"
#include "libuae.h"
#include "m68k.h"
#include "ti68k_def.h"

#define RAW_FLASH_SIZE (4u * 1024u * 1024u)
#define RUN_CHUNK_INSTRUCTIONS 100000
#define RUN_CHUNKS 200

static uint32_t fnv1a(const uint8_t *data, size_t length)
{
    uint32_t hash = UINT32_C(2166136261);
    size_t i;
    for (i = 0; i < length; i++) {
        hash ^= data[i];
        hash *= UINT32_C(16777619);
    }
    return hash;
}

static uint8_t *load_flash(const char *path)
{
    FILE *file = fopen(path, "rb");
    uint8_t *data;
    long size;
    if (!file) {
        fprintf(stderr, "error: cannot open %s: %s\n", path, strerror(errno));
        return NULL;
    }
    if (fseek(file, 0, SEEK_END) || (size = ftell(file)) < 0 ||
        fseek(file, 0, SEEK_SET)) {
        fprintf(stderr, "error: cannot size %s\n", path);
        fclose(file);
        return NULL;
    }
    if ((unsigned long)size != RAW_FLASH_SIZE) {
        fprintf(stderr, "error: expected %u-byte raw image, got %ld\n",
            RAW_FLASH_SIZE, size);
        fclose(file);
        return NULL;
    }
    data = (uint8_t *)malloc(RAW_FLASH_SIZE);
    if (!data || fread(data, 1, RAW_FLASH_SIZE, file) != RAW_FLASH_SIZE) {
        fprintf(stderr, "error: cannot read %s\n", path);
        free(data);
        fclose(file);
        return NULL;
    }
    fclose(file);
    return data;
}

int main(int argc, char **argv)
{
    uint32_t first_lcd_hash = 0;
    uint32_t last_lcd_hash = 0;
    unsigned lcd_changes = 0;
    int chunk;

    if (argc != 2) {
        fprintf(stderr, "usage: %s PRIVATE_FLASH.raw\n", argv[0]);
        return 2;
    }

    memset(&img_infos, 0, sizeof(img_infos));
    img_infos.data = load_flash(argv[1]);
    if (!img_infos.data) return 1;
    img_infos.calc_type = TI89t;
    memcpy(img_infos.version, "3.10", 5);
    img_infos.flash = FLASH_ROM;
    img_infos.has_boot = 0;
    img_infos.size = RAW_FLASH_SIZE;
    img_infos.hw_type = HW3;
    img_infos.rom_base = 0x80;
    img_loaded = 1;

    params.restricted = 0;
    params.cpu_rate = -1;
    params.hw_rate = -1;
    params.lcd_rate = -1;
    params.hw_protect = 1;
    params.recv_file = 0;

    if (hw_init() || hw_reset()) {
        fprintf(stderr, "error: TiEmu hardware initialization failed\n");
        return 1;
    }

    printf("BOOT initial_ssp=%08" PRIx32 " initial_pc=%08" PRIx32
           " pc=%08" PRIx32 "\n",
        tihw.initial_ssp, tihw.initial_pc, (uint32_t)m68k_getpc());

    for (chunk = 0; chunk < RUN_CHUNKS; chunk++) {
        uint32_t lcd_hash;
        int result = hw_m68k_run(RUN_CHUNK_INSTRUCTIONS, 0);
        tihw.lcd_ptr = (char *)&tihw.ram[tihw.lcd_adr & (tihw.ram_size - 1)];
        lcd_hash = fnv1a((const uint8_t *)tihw.lcd_ptr, 30u * 128u);
        if (chunk == 0) first_lcd_hash = lcd_hash;
        if (chunk && lcd_hash != last_lcd_hash) lcd_changes++;
        last_lcd_hash = lcd_hash;

        if (result || chunk % 10 == 0 || chunk == RUN_CHUNKS - 1) {
            printf("RUN chunk=%d result=%d pc=%08" PRIx32
                   " cycles=%u lcd_on=%d lcd_addr=%06" PRIx32
                   " lcd_fnv1a=%08" PRIx32 " changes=%u\n",
                chunk + 1, result, (uint32_t)m68k_getpc(),
                hw_m68k_get_cycle_count(0), tihw.on_off,
                tihw.lcd_adr, lcd_hash, lcd_changes);
        }
        if (result) break;
    }

    printf("RESULT pc=%08" PRIx32 " first_lcd=%08" PRIx32
           " last_lcd=%08" PRIx32 " lcd_changes=%u\n",
        (uint32_t)m68k_getpc(), first_lcd_hash, last_lcd_hash, lcd_changes);
    hw_exit();
    return lcd_changes ? 0 : 1;
}
