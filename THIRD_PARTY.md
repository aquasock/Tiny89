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
