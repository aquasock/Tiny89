/* SPDX-License-Identifier: MIT */
#ifndef TINY89_PLATFORM_H
#define TINY89_PLATFORM_H

#include <stdint.h>

#define T89_REG(offset) (*(volatile uint32_t *)(0xe8000000u + (offset)))

#define T89_UART_DATA    T89_REG(0x00)
#define T89_UART_STATUS  T89_REG(0x04)
#define T89_UART_RX      T89_REG(0x10)
#define T89_UART_CTRL    T89_REG(0x14)
#define T89_UART_LINK_MODE 0x1u /* CTRL: the line carries TinyTang's frames */
#define T89_LINK_STATUS  T89_REG(0x18)
#define T89_UART_TX_READY 0x01u
#define T89_UART_TX_IDLE  0x02u

/* The keyboard TinyTang forwards over the link, held by the desk link's input
 * frame parser (desk_link.sv): the low register is {usage3, usage2, usage1,
 * modifiers} and the high one is {sequence, usage6, usage5, usage4}.  Reading
 * the high register is enough to see whether a new report arrived. */
#define T89_KBD_LO       T89_REG(0x1c)
#define T89_KBD_HI       T89_REG(0x20)

#define T89_VIDEO_CTRL       T89_REG(0x10000)
#define T89_VIDEO_STATUS     T89_REG(0x10004)
#define T89_VIDEO_FRAMES     T89_REG(0x10008)
#define T89_VIDEO_ID         T89_REG(0x1000c) /* read: which video unit answers */
#define T89_VIDEO_ID_T89V    0x54383956u /* "T89V": the Tiny89 grayscale unit */
#define T89_VIDEO_ID_TNV1    0x544e5631u /* "TNV1": the inherited TinyNES unit */
#define T89_VIDEO_PALETTE(i) T89_REG(0x11000 + 4u * (i))
#define T89_VIDEO_FB_BASE    0x41fe0000u
#define T89_VIDEO_FB_BYTES   (128u << 10)
#define T89_VIDEO_FB(b)      ((uint8_t *)(T89_VIDEO_FB_BASE + 0x10000u * (b)))
#define T89_VIDEO_ENABLE     0x1u /* CTRL: show the frame buffers */
#define T89_VIDEO_BUFFER     0x2u /* CTRL: the buffer to show (STATUS: shown) */
#define T89_VIDEO_VBLANK     0x1u /* STATUS: a vertical blank is in progress */
#define T89_VIDEO_SHOWING    0x4u /* STATUS: the frame buffers are shown */
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
