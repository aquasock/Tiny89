/* SPDX-License-Identifier: GPL-2.0-or-later */
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "hw.h"
#include "images.h"
#include "libuae.h"
#include "m68k.h"
#include "tiny89_platform.h"
#include "ti68k_def.h"

#define FLASH_BYTES (4u * 1024u * 1024u)
#define INSTRUCTIONS_PER_CHUNK 1000000
#define CHUNKS 10

extern const uint8_t tiny89_flash_image[];
extern const uint8_t tiny89_flash_image_end[];

static uint32_t lcd_hash(void)
{
    const uint8_t *data = &tihw.ram[tihw.lcd_adr & (tihw.ram_size - 1)];
    uint32_t hash = UINT32_C(2166136261);
    size_t index;
    for (index = 0; index < 30u * 128u; index++) {
        hash ^= data[index];
        hash *= UINT32_C(16777619);
    }
    return hash;
}

static void show_lcd(void)
{
    const uint8_t *lcd = &tihw.ram[tihw.lcd_adr & (tihw.ram_size - 1)];
    uint8_t *frame = T89_VIDEO_FB(0);
    unsigned x, y;

    memset(frame, 0, 0x10000u);
    for (y = 0; y < 100; y++) {
        for (x = 0; x < 160; x++) {
            unsigned set = lcd[y * 30u + x / 8u] & (0x80u >> (x & 7));
            frame[(70u + y) * T89_VIDEO_WIDTH + 48u + x] = set ? 1 : 0;
        }
    }
    T89_VIDEO_PALETTE(0) = 0x9fb58fu;
    T89_VIDEO_PALETTE(1) = 0x182018u;
    t89_dcache_wbinval();
    T89_VIDEO_CTRL = T89_VIDEO_ENABLE;
}

int main(void)
{
    uint8_t *flash;
    uint32_t previous = 0;
    unsigned changes = 0;
    int chunk;

    printf("Tiny89 AE350 TiEmu smoke start\n");
    if ((size_t)(tiny89_flash_image_end - tiny89_flash_image) != FLASH_BYTES) {
        printf("BAD Tiny89 flash size=%" PRIu32 "\n",
            (uint32_t)(tiny89_flash_image_end - tiny89_flash_image));
        return 1;
    }
    flash = malloc(FLASH_BYTES);
    if (!flash) {
        printf("BAD Tiny89 flash allocation\n");
        return 1;
    }
    memcpy(flash, tiny89_flash_image, FLASH_BYTES);

    memset(&img_infos, 0, sizeof(img_infos));
    img_infos.data = flash;
    img_infos.calc_type = TI89t;
    memcpy(img_infos.version, "3.10", 5);
    img_infos.flash = FLASH_ROM;
    img_infos.has_boot = 0;
    img_infos.size = FLASH_BYTES;
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
        printf("BAD Tiny89 hardware initialization\n");
        return 1;
    }
    printf("Tiny89 BOOT SSP=%08" PRIx32 " PC=%08" PRIx32 "\n",
        tihw.initial_ssp, tihw.initial_pc);

    for (chunk = 0; chunk < CHUNKS; chunk++) {
        uint32_t hash;
        int result = hw_m68k_run(INSTRUCTIONS_PER_CHUNK, 0);
        hash = lcd_hash();
        if (chunk && hash != previous) changes++;
        previous = hash;
        show_lcd();
        printf("Tiny89 RUN N=%u R=%d PC=%08" PRIx32 " C=%u LCD=%08" PRIx32 "\n",
            (unsigned)(chunk + 1), result, (uint32_t)m68k_getpc(),
            hw_m68k_get_cycle_count(0), hash);
        if (result) {
            printf("BAD Tiny89 CPU result=%d\n", result);
            return 1;
        }
    }

    printf("Tiny89 READY PC=%08" PRIx32 " LCD=%08" PRIx32 " CHANGES=%u\n",
        (uint32_t)m68k_getpc(), previous, changes);
    hw_exit();
    return changes ? 0 : 1;
}
