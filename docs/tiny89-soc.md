# The Tiny89 AE350 SoC

Tiny89's fabric is TinyNES's AE350 platform, forked byte-faithfully. This page
is the fork record: where the files came from, the register map they implement,
and the one external fact the open flow cannot see for itself.

## Provenance

`fpga/tiny89_soc/` and `firmware/boot/` are copied with `cp` from
aquasock/TinyNES at commit `f5011dd374cfc65f4f17616fec97c903a66b6cc8`. That is
the commit verified in the TinyNES checkout. Every copied file keeps its
original `SPDX-License-Identifier: MIT` header; the licence is MIT and the
files stay MIT (`THIRD_PARTY.md`).

The forked files are, from TinyNES `fpga/ae350_soc/`:

| file | role |
| --- | --- |
| `ae350_soc.sv` | top module `ae350_soc_top`: AE350 instance, clocking, EXTS decode, NOBOOT report |
| `ahb_boot_rom.sv` | 32 KiB boot ROM on the ROM port (reset vector) |
| `ahb_ram.sv` | 64 KiB block RAM on the RAM (DDR_H) port |
| `ahb_sdram.sv` | the dock's 32 MiB SDRAM on J9, both CPU and video channels |
| `exts_uart.sv` | FPGA UART, 2,000,000 baud 8N1, registers in the EXTS window |
| `desk_link.sv` | UART link mode: TinyTang's frame protocol on the same line |
| `wii_pads.sv` | four Wii Classic Controllers on PMOD0 |
| `i2s_out.sv` | stereo I2S for a Pmod I2S2 on PMOD1, sample FIFO |
| `video.sv` | HDMI 1280x720 60 Hz out of two SDRAM frame buffers -- the starting point for Tiny89's own video unit, unmodified here |
| `ae350_soc.cst` | pins and IO types |
| `ae350_soc.sdc` | the measured clock constraints |
| `desk_font.hex` | the overlay's font image |

and from TinyNES `firmware/boot/`: `main.c`, `load.c`, `load.h`, `crt0.S`,
`link.ld`, `Makefile` -- the boot ROM whose banner now names Tiny89.

Tiny89 changes exactly two things and nothing else. The top module stays
`ae350_soc_top`; the boot ROM's banner reports `Tiny89 AE350 boot ROM` and
`Tiny89 AE350 C OK`/`Tiny89 SDRAM OK 32M`, while every load-protocol line the
loader matches (`TNL1`, the 16-byte header, 8192-byte blocks, CRC-32, ACK/NAK,
and the `TinyNES LOAD`/`TinyNES RUN`/`TinyNES EXIT` replies) is kept verbatim,
because TinyNES's `tools/load_ae350.py` and Tiny89's
`tools/deploy-ae350-probe.sh` both match those exact bytes. Inside `ae350_soc.sv`
only comments changed, and a fork header was added above TinyNES's platform
description. Every measured fact, pin, PLL site, clocking recipe and design
rule is TinyNES's and is preserved verbatim; `fpga/tiny89_soc/ae350_soc.cst`
and `ae350_soc.sdc` are byte-identical to TinyNES's.

`video.sv` is untouched. Replacing the inherited picture path with a
Tiny89-owned one is the next phase; the inherited unit shows a 256x240 frame
buffer at 3, up to 960 columns of 1280, which is why the calculator's 160x100
LCD appears as a 480x300 patch.

The `firmware/boot/Makefile` carries one comment-only edit: its "TinyNES AE350
boot ROM program" line and a `$readmemh` path comment that pointed at
`fpga/ae350_soc/ahb_boot_rom.sv` now name Tiny89 and
`fpga/tiny89_soc/ahb_boot_rom.sv`. No recipe, flag or variable changed.

## Register map

The EXTS window is decoded in `ae350_soc.sv` (the four selects are
`video_hit`, `pads_hit`, `audio_hit`, and UART otherwise) and the ports below
it are on the AE350's own AHB ports.

| unit | base | implementation | notes |
| --- | --- | --- | --- |
| UART | `0xe8000000` | `exts_uart.sv` | 2,000,000 baud 8N1; EXTS default (`!video_hit && !pads_hit && !audio_hit`) |
| video | `0xe8010000` | `video.sv` | registers, palette and emphasis in the EXTS window |
| Wii pads | `0xe8040000` | `wii_pads.sv` | `DATA(n)` report bytes 0..3, `DATA2(n)` bits 15:0 bytes 4 and 5 |
| I2S audio | `0xe8050000` | `i2s_out.sv` | sample FIFO |
| SDRAM | `0x40000000` | `ahb_sdram.sv` | 32 MiB on J9, shared by CPU and video channels |
| block RAM | `0x60000000` | `ahb_ram.sv` | 64 KiB on the RAM (DDR_H) port; the boot ROM's stack and data |
| boot ROM | `0x80000000` | `ahb_boot_rom.sv` | 32 KiB, reset vector (`crt0.S`: "Reset entry at 0x80000000") |

## The AE350 is a hard block

The AE350 itself is not fabric: it is a hard block in the GW5AST-138C
(device `GW5AST-LV138PG484AC1/I0`), with the A25 core and its AHB/APB fabric
inside. Its dedicated clock input is `PLL_R[0] CLKOUT1` on this part
(`ae350_soc.sv`). The block's port map and its timing live in the local
Apicula fork, `/run/media/vash/GIT/apicula-mathieufro` (sibling of this
repository, commit `cd3893f76f40bb972aa3053b1887b10d5b893e9e`):
`apycula/ae350_138c_outputs.py` and `apycula/ae350_138c_timing.py` describe
the block's ports and its timing, `fuzz/gw5ast138c/shapes/ae350_soc.py` is the
fabric-tap vehicle that separates the six clock nets the block needs, and
`tests/test_ae350_timing_138c.py` pins the 138C timings. The open flow has no
timing model for the AE350's ports, so each path to or from them is kept to at
most one LUT (`ae350_soc.sv`).

The fork also keeps the measured clocking rules verbatim, in
`fpga/tiny89_soc/ae350_soc.sdc`: `sys_clk` at 20.000 ns, `pix_clk5` at
2.694 ns and `pix_clk` at 13.468 ns, the last two being HDMI 720p's 371.25 MHz
serial clock and 74.25 MHz pixel clock. In `ae350_soc.sv` the UART divider is
`UART_DIV2 = 75` (2 * 75 MHz / 2,000,000 baud) and the SDRAM init delay is
`SDRAM_INIT_CYCLES = 15000` (200 us at 75 MHz). Placement decides pixel-clock
hold and bus-clock Fmax; the seed, the PLL-site choice and the
`NPNR_GOWIN_CLK_FIRST=bus_clk` rule that keeps the A25's bus off branch GB20
belong to the synthesis script, which a later phase owns. No build is run
here.

---

## The video unit is Tiny89's own

`fpga/tiny89_soc/video.sv` no longer carries TinyNES's NES picture. It scans a
160x100 one-byte-per-pixel image -- two buffers in the SDRAM's top 128 KiB,
buffer b at `0x41fe0000 + 0x10000*b`, pixel (x,y) at byte `256*y + x` with only
the first 160 bytes of each of the 100 rows carrying pixels -- and shows it at
a whole scale of 7 in both axes: 1120x700 centred at (80,10) in 1280x720,
square pixels, black bars of 80 columns and 10 rows. Seven is the largest whole
scale that fits the height (8 * 100 = 800 > 720); that, and the square pixels,
are this project's choice and not a sourced fact about the panel.

A pixel's palette index is its own value, 0 to 3, so the four entries the app
writes at `0xe8010000 + 0x11000 + 4*i` are the picture's four shades, paper to
ink. TinyNES's 768-entry palette and its per-line emphasis are gone; the
emphasis register still decodes, and a write to it is not an error, but nothing
reads it. The ASPECT register likewise stays at `0x1001c` and reads back, and
is otherwise a no-op, since a fixed scale has no aspect to choose. The register
map the app uses is otherwise unchanged: CTRL, STATUS and FRAMES at `0x10000`,
`0x10004` and `0x10008`. The ID register at `0x1000c` answers `0x54383956`,
"T89V", which is how the app tells this unit from the inherited one and takes
its grayscale path. The desk layer's 80x45 cell overlay is kept, font path and
all, so TinyDesk still draws over the picture.

`tools/build-tiny89-soc.sh` builds it with the open flow and no Gowin EDA. It
synthesises once, then places and routes eight seeds and packs the first whose
pixel clock has no hold violations and whose routing passes every gate: the
AE350's dedicated PLL hop, 75 MHz on the bus, 74.25 MHz on the pixel clock,
every video block RAM written and read on one clock and in a 9-bit mode, and
the pixel clock reaching the clock spines only through
`SPINE26 <= CBRIDGEOUT_BOTTOM2`. A sweep is not optional here: on this chip
some placements give the pixel clock hold violations, and the forked fabric
itself failed the check on five of the same eight seeds. Seed 1 passed, at
79.60 MHz on the bus and 86.53 on the pixel clock, and packed to
`build/tiny89_soc/tiny89_soc.bin`, SHA-256
`00ba0bf4e886cdfc7906d4fa6f542e730b043feebfd2e4ba5b495a0354c310c1`.

`tests/tiny89_video_tb.sv` checks the unit on its own outputs, driving the bus
side and the SDRAM video channel and comparing every screen pixel: the
geometry, the 7x7 mapping, each level against its palette entry, the bars, the
buffer flip taking effect only at a vertical blank, and the ID. It reports one
open disagreement -- the FRAMES register's rate against the benchmark's own
count of vertical blanks -- which is unresolved rather than passing.
