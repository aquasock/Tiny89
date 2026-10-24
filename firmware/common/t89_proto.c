/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * The TI-89 Titanium's link protocol, sender side.
 *
 * A packet is [target][command][length_lo][length_hi][data...][sum_lo][sum_hi]
 * with the sum being the 16-bit sum of the data bytes (libtifiles'
 * tifiles_checksum); a packet with no data is the four bytes
 * [target][command][0][0].  Sending one variable is the eight-step exchange
 * libticalcs' send_var() performs for the 68k family (cmd68k.cc, calc_9x.cc):
 *
 *     RTS  variable header          -> ACK from the calculator
 *                                   -> CTS from the calculator
 *     ACK                           -> the calculator is ready for data
 *     XDP  four zero bytes + data   -> ACK with a status word
 *     EOT  end of transmission      -> ACK
 *
 * The bytes reach the calculator through t89_link.c's queue, and this file
 * runs the emulated CPU while it waits: a transfer is not something that can
 * happen between frames, because the calculator's own code is what answers.
 * At 1 instruction per pump the emulator still executes in order, so a
 * transfer is deterministic and repeatable.
 */
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "m68k.h"
#include "t89_link.h"
#include "t89_proto.h"

/* Command numbers, dbus_pkt.h (libticalcs). */
#define CMD_RTS 0xC9u
#define CMD_CTS 0x09u
#define CMD_XDP 0x15u
#define CMD_ACK 0x56u
#define CMD_EOT 0x92u

/* Machine ids, dbus_pkt.h: the PC announces itself as 0x08 and the calculator
 * answers as 0x98. */
#define ID_PC  0x08u
#define ID_CALC 0x98u

/* How long to wait for an answer, in emulated instructions.  The calculator's
 * link code is interrupt driven and answers within a handful of instructions;
 * a million is far past any answer and short enough to fail in seconds. */
#define PROTO_WAIT_INSTRUCTIONS 2000000u

/* Pump the emulated machine until a byte arrives from the calculator. */
static int wait_byte(uint8_t *out, uint32_t instructions)
{
    for (uint32_t i = 0; i < instructions; i++) {
        if (t89_link_pending()) {
            int value = t89_link_read();
            if (value >= 0) {
                *out = (uint8_t)value;
                return 1;
            }
        }
        (void)hw_m68k_run(1, 0);
    }
    return 0;
}

/* Hand the calculator n bytes; false if it stopped taking them. */
static int send_bytes(const uint8_t *bytes, unsigned n)
{
    for (unsigned i = 0; i < n; i++) {
        unsigned stalled = 0;

        while (!t89_link_write(bytes[i])) {
            (void)hw_m68k_run(1, 0);
            if (++stalled > PROTO_WAIT_INSTRUCTIONS) {
                return 0;
            }
        }
        /* A byte sitting in the register is what raises the receive
         * interrupt; the calculator's ISR reads it out. */
        (void)hw_m68k_run(4, 0);
    }
    return 1;
}

static uint16_t data_sum(const uint8_t *data, unsigned n)
{
    uint16_t sum = 0;

    for (unsigned i = 0; i < n; i++) {
        sum = (uint16_t)(sum + data[i]);
    }
    return sum;
}

/* One packet whose data is two pieces end to end: the four zero bytes an XDP
 * carries before the variable, then the variable itself, which is far too
 * large to copy into a buffer of its own. */
static int send_packet_parts(uint8_t command, const uint8_t *a, unsigned na,
                             const uint8_t *b, unsigned nb)
{
    const unsigned n = na + nb;
    uint8_t header[4];
    uint8_t tail[2];
    uint16_t sum = data_sum(a, na);

    sum = (uint16_t)(sum + data_sum(b, nb));
    header[0] = ID_PC;
    header[1] = command;
    header[2] = (uint8_t)(n & 0xffu);
    header[3] = (uint8_t)((n >> 8) & 0xffu);
    if (!send_bytes(header, sizeof header) || !send_bytes(a, na) ||
        !send_bytes(b, nb)) {
        return 0;
    }
    tail[0] = (uint8_t)(sum & 0xffu);
    tail[1] = (uint8_t)((sum >> 8) & 0xffu);
    return send_bytes(tail, sizeof tail);
}

/* One packet.  A zero length sends the four-byte short form, which is what
 * ACK, EOT and CTS are. */
static int send_packet(uint8_t command, const uint8_t *data, unsigned n)
{
    uint8_t header[4];
    uint8_t tail[2];
    const uint16_t sum = data_sum(data, n);

    header[0] = ID_PC;
    header[1] = command;
    header[2] = (uint8_t)(n & 0xffu);
    header[3] = (uint8_t)((n >> 8) & 0xffu);
    if (!send_bytes(header, sizeof header)) {
        return 0;
    }
    if (n != 0) {
        if (!send_bytes(data, n)) {
            return 0;
        }
        tail[0] = (uint8_t)(sum & 0xffu);
        tail[1] = (uint8_t)((sum >> 8) & 0xffu);
        if (!send_bytes(tail, sizeof tail)) {
            return 0;
        }
    }
    return 1;
}

/* One reply from the calculator: the command byte, and up to `room` data
 * bytes.  A four-byte short packet carries no data; a longer one carries the
 * length in its header and a two-byte sum. */
static int recv_packet(uint8_t *command, uint8_t *data, unsigned room,
                       unsigned *got)
{
    uint8_t header[4];
    unsigned n;
    uint16_t sum;

    for (unsigned i = 0; i < sizeof header; i++) {
        if (!wait_byte(&header[i], PROTO_WAIT_INSTRUCTIONS)) {
            return 0;
        }
    }
    *command = header[1];
    n = (unsigned)header[2] | ((unsigned)header[3] << 8);
    *got = 0;
    if (n == 0) {
        return 1;
    }
    if (n > room) {
        /* Longer than this caller can hold: drain it and report failure so a
         * caller cannot mistake a truncated packet for a whole one. */
        for (unsigned i = 0; i < n + 2u; i++) {
            uint8_t scratch;
            (void)wait_byte(&scratch, PROTO_WAIT_INSTRUCTIONS);
        }
        return 0;
    }
    for (unsigned i = 0; i < n; i++) {
        if (!wait_byte(&data[i], PROTO_WAIT_INSTRUCTIONS)) {
            return 0;
        }
    }
    sum = data_sum(data, n);
    for (unsigned i = 0; i < 2; i++) {
        uint8_t byte;
        if (!wait_byte(&byte, PROTO_WAIT_INSTRUCTIONS)) {
            return 0;
        }
        if (byte != (uint8_t)((sum >> (8 * i)) & 0xffu)) {
            return 0;
        }
    }
    *got = n;
    return 1;
}

/* Wait for one command, saying which one arrived when it is not the one
 * wanted: the calculator's refusals (SKP, ERR) are the useful part. */
static int expect(uint8_t wanted, const char *what)
{
    uint8_t command = 0;
    uint8_t data[8];
    unsigned got = 0;

    if (!recv_packet(&command, data, sizeof data, &got)) {
        printf("Tiny89 PROTO %s: no answer\n", what);
        return 0;
    }
    if (command != wanted) {
        printf("Tiny89 PROTO %s: got %02x%s\n", what, command,
               got >= 3 ? " (refused)" : "");
        return 0;
    }
    return 1;
}

/* Send one variable.  The header the calculator matches is
 * [size:4][type][name length][name][0], with the size little-endian over its
 * low word first -- libticalcs' ti89_send_REQ/RTS build it exactly so. */
int t89_proto_send_var(uint8_t type, const char *name, const uint8_t *data,
                       unsigned size)
{
    static const uint8_t zeros[4] = { 0, 0, 0, 0 };
    uint8_t header[32];
    unsigned name_len = (unsigned)strlen(name);

    if (name_len > 8u || size == 0) {
        printf("Tiny89 PROTO var refused locally\n");
        return 0;
    }

    header[0] = (uint8_t)(size & 0xffu);
    header[1] = (uint8_t)((size >> 8) & 0xffu);
    header[2] = (uint8_t)((size >> 16) & 0xffu);
    header[3] = (uint8_t)((size >> 24) & 0xffu);
    header[4] = type;
    header[5] = (uint8_t)name_len;
    memcpy(header + 6, name, name_len);
    header[6 + name_len] = 0x00;

    printf("Tiny89 PROTO RTS %s type %02x size %u\n", name, type, size);
    if (!send_packet(CMD_RTS, header, 6u + name_len + 1u)) {
        printf("Tiny89 PROTO RTS not sent\n");
        return 0;
    }
    if (!expect(CMD_ACK, "RTS/ACK")) {
        return 0;
    }
    if (!expect(CMD_CTS, "CTS")) {
        return 0;
    }
    if (!send_packet(CMD_ACK, NULL, 0)) {
        printf("Tiny89 PROTO ACK not sent\n");
        return 0;
    }

    /* The data packet's first four bytes are zero, and the calculator's copy
     * of the variable begins after them.  Sent as two pieces so an app larger
     * than any buffer still travels. */
    if (!send_packet_parts(CMD_XDP, zeros, sizeof zeros, data, size)) {
        printf("Tiny89 PROTO XDP not sent\n");
        return 0;
    }
    if (!expect(CMD_ACK, "XDP/ACK")) {
        return 0;
    }
    if (!send_packet(CMD_EOT, NULL, 0)) {
        printf("Tiny89 PROTO EOT not sent\n");
        return 0;
    }
    if (!expect(CMD_ACK, "EOT/ACK")) {
        return 0;
    }
    printf("Tiny89 PROTO %s accepted\n", name);
    return 1;
}
