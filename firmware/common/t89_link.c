/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * The calculator's link port, as far as the emulated machine can tell: two
 * open-collector wires and a one-byte register in each direction.
 *
 * ti_hw/ports.c reaches the outside world through four calls it expects from a
 * cable driver -- ticables_cable_set_d0/d1 and ticables_cable_get_d0/d1 on a
 * port write and a port read -- and through the four function pointers
 * ti_hw/dbus.c would install (hw_dbus_putbyte, hw_dbus_getbyte,
 * hw_dbus_checkread, hw_dbus_reinit) for the byte register at 0x0f and the
 * status bits at 0x0d.  Until now both sets were stubs: the wires went nowhere
 * and no byte could cross, which is why software could only reach this
 * calculator by being built into its flash image.
 *
 * This file is that driver.  It models the wires as open collector -- a line
 * reads high only when neither end pulls it low -- and keeps a queue each way,
 * so the bytes the calculator transmits can be read and the bytes it should
 * receive can be handed to it.  What it does not do is speak: the TI protocol
 * (the CTS/ACK handshake and the variable packets) is the caller's, and the
 * first caller is main.c's trace, whose job is to show what the calculator
 * itself sends when its Link screen is opened.
 */
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "ports.h"
#include "ti68k_def.h"

/* The four hooks ti_hw/dbus.c owns in a full build.  They are function
 * pointers rather than calls so a cable can be swapped for another; here the
 * only cable is this file, installed by t89_link_install(). */
extern void (*hw_dbus_reinit)(void);
extern void (*hw_dbus_putbyte)(uint8_t value);
extern uint8_t (*hw_dbus_getbyte)(void);
extern int (*hw_dbus_checkread)(void);

/* The wire state: 1 is an undriven (high) line, 0 a line pulled low.  Both
 * ends are drivers on the same pair, which is what open collector means. */
static uint8_t s_calc_d0 = 1;    /* the calculator's own drive */
static uint8_t s_calc_d1 = 1;
static uint8_t s_ours_d0 = 1;    /* this side's drive */
static uint8_t s_ours_d1 = 1;

/* What the calculator has transmitted (0x0f writes), oldest first. */
#define T89_LINK_RX_BYTES 1024u
static uint8_t s_rx[T89_LINK_RX_BYTES];
static unsigned s_rx_head;
static unsigned s_rx_tail;

/* What the calculator should receive, oldest first. */
#define T89_LINK_TX_BYTES 1024u
static uint8_t s_tx[T89_LINK_TX_BYTES];
static unsigned s_tx_head;
static unsigned s_tx_tail;

/* Counters, so a run can say whether the calculator touched the port at all. */
static unsigned long s_wire_writes;
static unsigned long s_wire_reads;
static unsigned long s_bytes_out;    /* calculator -> here */
static unsigned long s_bytes_in;     /* here -> calculator */

/* ---- the wires ---- */

void ticables_cable_set_d0(CableHandle *handle, int value)
{
    (void)handle;
    s_calc_d0 = value ? 1 : 0;
    s_wire_writes++;
}

void ticables_cable_set_d1(CableHandle *handle, int value)
{
    (void)handle;
    s_calc_d1 = value ? 1 : 0;
    s_wire_writes++;
}

int ticables_cable_get_d0(CableHandle *handle)
{
    (void)handle;
    s_wire_reads++;
    return s_calc_d0 & s_ours_d0;
}

int ticables_cable_get_d1(CableHandle *handle)
{
    (void)handle;
    return s_calc_d1 & s_ours_d1;
}

/* ---- the byte register, calculator -> here ---- */

static void link_putbyte(uint8_t value)
{
    const unsigned next = (s_rx_head + 1u) % T89_LINK_RX_BYTES;

    s_bytes_out++;
    if (next != s_rx_tail) {
        s_rx[s_rx_head] = value;
        s_rx_head = next;
    }
    /* The transmit register is empty again as soon as the byte is taken; the
     * calculator polls STX (0x0d bit 0) before it writes the next one, and a
     * driver that never set this would hang the first transfer. */
    io_bit_set(0x0d, 6);
    io_bit_set(0x0d, 2);
}

/* ---- the byte register, here -> calculator ---- */

static uint8_t link_getbyte(void)
{
    uint8_t value = 0;

    if (s_tx_tail != s_tx_head) {
        value = s_tx[s_tx_tail];
        s_tx_tail = (s_tx_tail + 1u) % T89_LINK_TX_BYTES;
        s_bytes_in++;
    }
    io_bit_clr(0x0d, 5);
    return value;
}

static int link_checkread(void)
{
    if (s_tx_tail != s_tx_head) {
        io_bit_set(0x0d, 5);    /* SRX: the receive register is full */
        io_bit_set(0x0d, 2);    /* link activity */
        return 1;
    }
    return 0;
}

static void link_reinit(void)
{
    s_rx_head = s_rx_tail = 0;
    s_tx_head = s_tx_tail = 0;
}

/* ---- what the program uses ---- */

void t89_link_install(void)
{
    hw_dbus_reinit = link_reinit;
    hw_dbus_putbyte = link_putbyte;
    hw_dbus_getbyte = link_getbyte;
    hw_dbus_checkread = link_checkread;
    link_reinit();
}

/* The calculator's first byte, or -1 when it has sent nothing. */
int t89_link_read(void)
{
    uint8_t value;

    if (s_rx_tail == s_rx_head) {
        return -1;
    }
    value = s_rx[s_rx_tail];
    s_rx_tail = (s_rx_tail + 1u) % T89_LINK_RX_BYTES;
    return value;
}

/* Hand the calculator a byte; false when its queue is full. */
int t89_link_write(uint8_t value)
{
    const unsigned next = (s_tx_head + 1u) % T89_LINK_TX_BYTES;

    if (next == s_tx_tail) {
        return 0;
    }
    s_tx[s_tx_head] = value;
    s_tx_head = next;
    return 1;
}

int t89_link_pending(void)
{
    return s_rx_tail != s_rx_head;
}

/* One line for the console: has the calculator touched the port, and has it
 * said anything.  The counters are cumulative for the session. */
void t89_link_report(const char *when)
{
    printf("Tiny89 LINK %s wires %lu writes %lu reads, %lu bytes out, %lu in\n",
           when, s_wire_writes, s_wire_reads, s_bytes_out, s_bytes_in);
}
