# Third-party software and material

## TiEmu and UAE

Tiny89 fetches TiEmu from `debrouxl/tiemu` at commit
`2b8df9280d2054e11ffcf7e57a73899b3ce7a7e5`. The fetched source is not stored
in this repository. The AE350 and host diagnostics link TiEmu's UAE MC68000
engine and calculator hardware modules. TiEmu's `README.linux` designates GNU
GPL version 2; its bundled UAE engine also carries the GPL version 2 text.

`tools/convert_89u.py` implements the private raw-image layout used by
TiEmu's `ti68k_convert_tib_to_image` in `tiemu/trunk/src/core/images.c`.
Tiny89's TiEmu adapter, probes, converter, and combined emulator program are
therefore distributed under GPL-2.0-only. Copyright notices in the fetched
upstream files remain intact.

Upstream: https://github.com/debrouxl/tiemu

## TinyNES platform runtime

The AE350 startup, linker layout, UART/newlib runtime and platform register
interface under `firmware/` are adapted from aquasock/TinyNES commit
`f5011dd374cfc65f4f17616fec97c903a66b6cc8`, whose corresponding files are
MIT-licensed. They were renamed and reduced for Tiny89; their SPDX identifiers
remain MIT.

Upstream: https://github.com/aquasock/TinyNES/tree/f5011dd374cfc65f4f17616fec97c903a66b6cc8

## TinyNES AE350 SoC fabric and boot ROM

`fpga/tiny89_soc/` and `firmware/boot/` are copied unmodified with `cp` from
aquasock/TinyNES at commit `f5011dd374cfc65f4f17616fec97c903a66b6cc8`, which is
the commit verified in that checkout. Every copied file keeps its original
header and licence: the `.sv`, `.c`, `.h`, `.S`, `.ld` and `Makefile` files
carry TinyNES's unchanged `SPDX-License-Identifier: MIT` line, and
`desk_font.hex` keeps its own "font8x8_basic by Daniel Hepper, public domain
(tools/fetch_font8x8.sh)" comment. `ae350_soc.cst` and `ae350_soc.sdc` carry
no licence header upstream either and are byte-identical here. Copied paths,
TinyNES `fpga/ae350_soc/` and `firmware/boot/`:

- `fpga/tiny89_soc/`: `ae350_soc.sv`, `ahb_boot_rom.sv`, `ahb_ram.sv`,
  `ahb_sdram.sv`, `exts_uart.sv`, `desk_link.sv`, `wii_pads.sv`, `i2s_out.sv`,
  `video.sv`, `ae350_soc.cst`, `ae350_soc.sdc`, `desk_font.hex`;
- `firmware/boot/`: `main.c`, `load.c`, `load.h`, `crt0.S`, `link.ld`,
  `Makefile`;
- `firmware/boot/support/`: `tinynes.h` (from TinyNES `firmware/include/`),
  `memtest.h`, `memtest.c`, `uart.h`, `uart.c` (from TinyNES
  `firmware/common/`). These five are byte-identical copies and are the
  register contract and the UART and memory helpers the forked boot ROM is
  written against; a later phase repointed `firmware/boot/Makefile`'s include
  and source paths at them. They live here rather than under
  `firmware/common/`, which already holds the running app's own UART with a
  different interface.

Tiny89 changes exactly two things: the video unit (owned by a later phase) and
the boot ROM's banner, which now names Tiny89. The top module stays
`ae350_soc_top`; the UART load protocol, the register map, the pins, the PLL
sites and the measured clocking and design rules are TinyNES's and are
preserved. `fpga/tiny89_soc/ae350_soc.cst` and `ae350_soc.sdc` are byte-identical to
TinyNES's; `firmware/boot/Makefile` differs only in two comment lines, named
in `docs/tiny89-soc.md`.

Upstream: https://github.com/aquasock/TinyNES/tree/f5011dd374cfc65f4f17616fec97c903a66b6cc8

## Graph89

Graph89 commit `659c8da89699cb7acecb394ea8d79d69dc9474bb` was consulted as evidence that a
headless TiEmu library can run on a non-desktop ARM target. No Graph89 source
was copied into Tiny89. Graph89 as a whole is GPL-3.0-or-later.

Upstream: https://github.com/milaq/graph89

## TI operating-system image

TI-89 Titanium operating-system upgrades are not distributed by Tiny89. The
user supplies an upgrade they are entitled to use. `.89u`, `.img`, `.rom`, and
`.raw` files are ignored by Git. The hardware-proven development input had
SHA-256 `39440a0864b936520026fc674f9b35260405d8696fcfcc7a2770b6f79b6a80ee`;
neither it nor its generated raw image is part of the repository.
