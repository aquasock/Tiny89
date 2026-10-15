/* SPDX-License-Identifier: MIT */
#ifndef TINY89_PLATFORM_H
#define TINY89_PLATFORM_H

#include <stdint.h>

#define T89_REG(offset) (*(volatile uint32_t *)(0xe8000000u + (offset)))

#define T89_UART_DATA    T89_REG(0x00)
#define T89_UART_STATUS  T89_REG(0x04)
#define T89_UART_TX_READY 0x01u
#define T89_UART_TX_IDLE  0x02u

#define T89_VIDEO_CTRL       T89_REG(0x10000)
#define T89_VIDEO_STATUS     T89_REG(0x10004)
#define T89_VIDEO_PALETTE(i) T89_REG(0x11000 + 4u * (i))
#define T89_VIDEO_FB_BASE    0x41fe0000u
#define T89_VIDEO_FB_BYTES   (128u << 10)
#define T89_VIDEO_FB(b)      ((uint8_t *)(T89_VIDEO_FB_BASE + 0x10000u * (b)))
#define T89_VIDEO_ENABLE     0x1u
#define T89_VIDEO_WIDTH      256u
#define T89_VIDEO_HEIGHT     240u

#define T89_CORE_HZ 750000000u
#define T89_MCCTL_COMMAND 0x7cc
#define T89_MCCTL_L1D_WBINVAL_ALL 6u

static inline void t89_dcache_wbinval(void)
{
    uint32_t command = T89_MCCTL_L1D_WBINVAL_ALL;
    __asm__ volatile("csrw 0x7cc, %0" :: "r"(command) : "memory");
}

static inline uint64_t t89_cycles64(void)
{
    uint32_t high, low, again;
    do {
        __asm__ volatile("csrr %0, mcycleh" : "=r"(high));
        __asm__ volatile("csrr %0, mcycle" : "=r"(low));
        __asm__ volatile("csrr %0, mcycleh" : "=r"(again));
    } while (high != again);
    return (uint64_t)high << 32 | low;
}

void t89_put_char(char value);
void t89_put_string(const char *value);
void t89_wait_tx_idle(void);

#endif
