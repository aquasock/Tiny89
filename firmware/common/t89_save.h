/* SPDX-License-Identifier: MIT */
/*
 * The calculator's RAM, kept on the card.
 *
 * What a TI-89 Titanium keeps across a power-off is its RAM: the variables and
 * the state the OS holds there.  In this emulator that is tihw.ram, allocated
 * and zeroed once by TiEmu (core/ti_hw/mem.c, at the size mem_size.h gives
 * this model), and hw_reset() does not clear it -- so a snapshot written into
 * it before the CPU first runs is exactly what a battery-backed calculator has
 * at power-up.
 *
 * The board's half of this already exists: TinyNES's NVRAM protocol, which
 * TinyTang's ae350run implements.  A program asks for the file with
 * "TinyNES NVRAM WANT <address> <room>" and receives it headerless at that
 * address, straight into the buffer it names; it hands one back by printing
 * "TinyNES NVRAM SAVE <length> <crc32>" and then the bytes as hex lines, which
 * ae350run decodes into a binary .sav and checks, discarding it if the length
 * or the CRC disagree.
 *
 * Hex is the wire's requirement rather than a choice: in link mode the line
 * carries text frames, and a line ends only at a newline, so binary cannot go
 * as it is and four bits to a hex digit leave neither.
 */
#ifndef T89_SAVE_H
#define T89_SAVE_H

#include <stddef.h>
#include <stdint.h>

/* The calculator's RAM, mem_size.h's RAM_SIZE_TI89T. */
#define T89_SAVE_BYTES (256u * 1024u)
/* TinyNES's line: 32 bytes as 64 hex digits, then the newline that frames it. */
#define T89_SAVE_LINE_BYTES 32u
#define T89_SAVE_LINE_CHARS (2u * T89_SAVE_LINE_BYTES)

/* CRC-32 as ae350run checks it with (ae350_load.c's ae350_crc32): the
 * reflected CRC-32 of CRC-32/ISO-HDLC, which is zlib's and Ethernet's.  Start
 * at 0.  A save's CRC travels in its header line and again at the end, so a
 * byte lost in the link is a save the board throws away instead of one that
 * loads as a broken calculator next time. */
uint32_t t89_save_crc32(uint32_t crc, const uint8_t *data, size_t length);

/* `n` bytes as 2n lower-case hex digits and a terminator.  The newline is the
 * caller's, because the pacing that keeps the board able to file the line sits
 * between it and the next one. */
void t89_save_hex(char *out, const uint8_t *bytes, size_t n);

/* True when every byte is the same, which is what RAM the calculator never
 * wrote looks like.  Worth saying out loud in a log line: from the card a
 * blank save and a real one are the same file. */
int t89_save_is_blank(const uint8_t *data, size_t length);

#endif
