## 1 COMMIT Unreleased 2026-10-15T05:02:13-07:00

#### Coming From:

Unreleased c7949f5

#### Purpose:

Create Tiny89's authoritative technical reference from the TinyNES template and seed it with relevant evidence from TinyNES, TinyCreature and TinyTang.

#### Outcome:

`.ai/core-reference.md` now follows TinyNES's record schema, routing, index, record template and maintenance boundary; reserves project-specific topics for the TI-89 Titanium CPU, ASIC, memory, display, keyboard, I/O, operating system and conformance evidence without inventing facts; and records exact commit-pinned routes to TinyNES's measured board, FPGA, memory, HDMI, toolchain and core-link facts, TinyCreature's conditional AE350/fabric measurements, TinyTang's live integration source and its colibri provenance record. No calculator specification was available locally, so none was asserted. The core-syntax audit re-read `.ai/core.md` and `.ai/core-syntax.md`, inspected the complete `.ai/` diff, confirmed `.ai/core.md` was unchanged, found one new sequential entry with exactly the required six sections and allowed Status values, and confirmed the active log contains one entry.

#### Next Steps:

Obtain and open authoritative TI-89 Titanium, MC68000 and component documentation, then add narrowly scoped source records before choosing the emulator architecture or implementing calculator-visible behaviour.

#### Files Modified:

None.

#### Status:

- Build: N/A
- Deployment: N/A
- User Test: N/A

---

## 2 COMMIT Unreleased 2026-10-15T05:48:42-07:00

#### Coming From:

Unreleased f9800f9

#### Purpose:

Port the headless TiEmu TI-89 Titanium core to the AE350 and prove that the supplied operating-system image boots on the Tang Console 138K.

#### Outcome:

Tiny89 now deterministically fetches TiEmu `2b8df9280d2054e11ffcf7e57a73899b3ce7a7e5`, converts the private TI-89 Titanium OS 3.10 upgrade into an ignored 4 MiB raw image, and builds the retained MC68000 and calculator-hardware core for both the host and RV32IMAC AE350. The host probe ran 20 million interpreted instructions, observed 11 LCD changes, and settled at PC `0x00821e46` with LCD FNV-1a `0c0df297`; converter tests and the RV32 compile probe passed. The AE350 ELF has SHA-256 `f4b2629817d26e1287361c9580f59ca89a1f98fddb8a402dbce281045ab7e835`, contains 4,625,618 bytes of text, 34,576 bytes of initialized data and 455,636 bytes of BSS, and produces a 4,660,196-byte load image at `0x40000000`. The inherited AE350 FPGA image was stored on the Tang at `/root/cores/console138k/tiny89-ae350.bin`; its SDRAM self-test passed, TinyTang loaded the ELF, and ten one-million-instruction slices ended at the same PC and LCD hash with `Tiny89 READY` and exit zero. The user reported that the TI-89 Apps desktop appeared on HDMI. Licensing and provenance are recorded, while the proprietary OS input, generated raw image, fetched upstream tree and build outputs remain outside Git. The required core-syntax audit re-read `.ai/core.md` and `.ai/core-syntax.md`, inspected the complete `.ai/` diff, confirmed `.ai/core.md` was unchanged, validated this sequential entry's six sections and allowed Status values, and confirmed the active log contains two entries.

#### Next Steps:

Add persistent calculator state, controller-to-keyboard input mapping and continuous frame execution, then replace the temporary inherited AE350 platform image with a Tiny89-owned FPGA build and validate each capability on the Tang.

#### Files Modified:

- .gitignore
- LICENSE
- README.md
- THIRD_PARTY.md
- docs/tiemu-ae350-port.md
- firmware/apps/tiemu_smoke/Makefile
- firmware/apps/tiemu_smoke/flash.S
- firmware/apps/tiemu_smoke/main.c
- firmware/common/uart.c
- firmware/include/tiny89_platform.h
- firmware/rt/crt0.S
- firmware/rt/link.ld
- firmware/rt/syscalls.c
- src/host/boot_probe.c
- src/port/dbg_bkpts.h
- src/port/gettimeofday.h
- src/port/glib.h
- src/port/intl.h
- src/port/logging.h
- src/port/ticables.h
- src/port/ticalcs.h
- src/port/ticonv.h
- src/port/tiemu_port.c
- src/port/tifiles.h
- src/port/tilibs.h
- tests/test_convert_89u.py
- tools/build-ae350-probe.sh
- tools/build-host-probe.sh
- tools/convert_89u.py
- tools/deploy-ae350-probe.sh
- tools/fetch-tiemu.sh
- tools/probe-rv32.sh

#### Status:

- Build: PASS
- Deployment: PASS
- User Test: PASS

---
