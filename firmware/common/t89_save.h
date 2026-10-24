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
 * "TinyNES NVRAM SAVE <length> <crc32>" and then the bytes as hex lines,
 * which ae350run decodes into a binary .sav and checks when the DONE line
 * arrives, discarding the file if the length or the CRC disagree.
 *
 * Hex is TinyNES's shape and it is also the only one that has produced a valid
 * file in the user's configuration.  A binary payload was built and is faster
 * -- the bytes themselves, 128 to a frame, tens of kilobytes a second -- and it
 * worked in every run measured on the bench, but three saves from the user's
 * own procedure produced no file: each reported the whole 262144 bytes, nothing
 * dropped in the link ring, and a CRC that disagreed with the header.  Every
 * byte arrived and they were not the same bytes, at both 8 KB/s and 32 KB/s.
 * What that was is measured and recorded in core-log entry 14: frames of a
 * payload go missing without either end counting them, and a burst's opening
 * is where it happens, so the save warms the payload with empty lines and
 * gives the board more room between lines.  With both, the full 262144 bytes
 * arrive whole and the CRC agrees at both ends.  Hex stays the payload because
 * it is the shape that has produced a valid file.
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
/* How long the board is given to file one line: four milliseconds, TinyNES's
 * own measured figure on this board, which is about 8 KB/s.  The rate is one
 * number to raise once a measurement from the user's side justifies it; what
 * has been measured there is that a faster payload did not survive.
 *
 * Raised to eight on 2026-10-16, and the measurement is the justification: an
 * 8192-byte save lost exactly three lines, with nothing counted as dropped at
 * either end, so the 4 ms figure is not enough room in this configuration.  It
 * is the one number that changes, so if a save lands now and did not at four,
 * this is what did it; if it still does not, the pause is not the mechanism
 * and this goes back. */
#define T89_SAVE_PACE_MS 8u

/* The board files the payload 512 bytes at a time, and a card write holds the
 * CPU with interrupts masked while it runs, so every byte that arrives during
 * it is lost -- the BL616's UART receive FIFO is 32 bytes and its link ring is
 * not being drained while the card has the core.  Measured on 2026-10-23: a
 * save either arrived whole and verified, or arrived the right length with the
 * wrong CRC, because the board counts its payload down by length and the
 * program's own trailing text fills the missing bytes back up.
 *
 * So the pause is aligned to the board's chunk rather than to the frame: every
 * T89_SAVE_CHUNK_BYTES the sender stops long enough for one card write to
 * finish.  This costs about fifteen seconds on 256 KiB and is the whole point
 * of the figure below. */
#define T89_SAVE_CHUNK_BYTES 512u
/* Thirty rather than twenty as of 2026-10-24: a 263 KB state save still came
 * back short in one session out of three, with the card never holding the CPU
 * longer than 3 ms, so the margin was not the card's write but where the write
 * landed relative to the sender.  The cost over 256 KiB is about five seconds. */
#define T89_SAVE_CHUNK_MS 30u

/* The pause between the header line and the first payload byte.  Parsing the
 * header is when the board does its card work -- it opens the .part, appends
 * the arm line to last.log, opens the file -- and each of those holds the CPU
 * with interrupts masked.  Measured on 2026-10-24: with only the 8 ms line
 * pause here, a 263 KB state arrived the right length with the wrong CRC on
 * every attempt, and the bytes lost were the ones sent while the board was
 * opening the file.  Four hundred milliseconds is longer than the whole arm
 * sequence takes and costs a third of a second on a save that takes twenty
 * seconds. */
#define T89_SAVE_ARM_MS 400u

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
