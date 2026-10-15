/* SPDX-License-Identifier: MIT */
#include "tiny89_platform.h"

void t89_put_char(char value)
{
    while (!(T89_UART_STATUS & T89_UART_TX_READY)) {}
    T89_UART_DATA = (uint8_t)value;
}

void t89_put_string(const char *value)
{
    while (*value) t89_put_char(*value++);
}

void t89_wait_tx_idle(void)
{
    while (!(T89_UART_STATUS & T89_UART_TX_IDLE)) {}
}
