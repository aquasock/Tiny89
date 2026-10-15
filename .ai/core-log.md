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

## 3 COMMIT Unreleased 2026-10-15T06:27:10-07:00

#### Coming From:

Unreleased 6256eb6

#### Purpose:

Turn the AE350 diagnostic from a ten-million-instruction smoke test into a continuous session that presents the calculator's LCD at the platform's vertical blank and takes its keypad from a Wii Classic Controller, and prove both on the Tang.

#### Outcome:

`firmware/apps/tiemu_smoke/main.c` now runs one 60 Hz frame of emulated time per iteration -- 200,000 MC68000 cycles, the TI-89 Titanium's 12 MHz OSC1 divided by 60 -- looping forever, sampling controller 0 each frame, presenting the LCD into alternating frame buffers when the fabric reports a new vertical blank, printing `Tiny89 READY` exactly once and a sixty-frame heartbeat carrying the frame number, program counter, cycle count, LCD hash, vertical-blank count, pad word, keys held and layer, and never returning except on an unrecoverable fault. `firmware/common/t89_pad.h` and `firmware/common/t89_pad.c` add the decode of the inherited fabric's pad registers and a three-layer mapping (nav, digits, math, cycled by ZL) from the fifteen active-low buttons and the left stick to TiEmu's `TiKey` values, with `t89_pad_none()` as the single place the no-controller state is written down; the mapping is one table. `firmware/include/tiny89_platform.h` gained the fabric's video and pad register block, `firmware/apps/tiemu_smoke/Makefile` the pad source and its include path, and `tests/run_t89_pad_test.sh` with `tests/test_t89_pad.c` host-test the mapping in 205 checks. The build produced 4,627,200 bytes of text, 34,584 of initialized data and 455,636 of BSS, and the deployed ELF has SHA-256 `8459e356074a92bde2a7da4837c146be9b4a4d3e494948f619a7dc2cb3068045`. On the board the running FPGA core was first found not to be Tiny89's, so `tiny89-ae350.bin` was loaded with TinyTang's `tangload` and the image then transferred at 186.5 KB/s; the session reported the same boot vectors as the host probe (`SSP=0x00004c00`, `PC=0x00812188`), the pad unit answering as `544e5031`, and 480 presented frames at about one per vertical blank, reaching the same `0x00821e46` and `0c0df297` desktop state, with the user confirming that the controller's mapping works. Reading the two pieces against each other and against the fabric's `wii_pads.sv` caught two defects before deployment: a zeroed pad structure had both stick axes at their minimum, which the direction pass reads as up and left held, fixed by the centred `t89_pad_none()` on both absent paths; and gating the poll on the reads-completed field, which the fabric advances only on a successful transfer, would have left the last sample's keys pressed once a controller stopped answering, fixed by sampling every frame. An independent verifier returned FAIL on the first revision for the second of those, having passed the other six checks, and the corrected revision was verified only by the author. The host boot probe still exits zero with eleven LCD changes and `RESULT pc=00821e46 first_lcd=5847a245 last_lcd=0c0df297`, and `tools/deploy-ae350-probe.sh` no longer expects the loader's own pass: a continuous program never exits, so the script now bounds the run, requires its marker and rejects a `BAD` line, and names the `tangload` step whose absence is what a missing boot ROM on the pipe means. The required core-syntax audit re-read `.ai/core.md` and `.ai/core-syntax.md`, inspected the complete `.ai/` diff, confirmed that `.ai/core.md` was unchanged, validated this sequential entry's six sections, prose, Status values and the log's three-entry count, and confirmed that no settled entry was rewritten.

#### Next Steps:

Hold input work at this state, and build a Tiny89-owned AE350 platform image whose video path shows the calculator's 160x100 LCD filling the 720p output, since the inherited fabric centers it at 1:1 in a 256x240 frame buffer and caps the picture at 960 of 1280 columns; decide the scaling geometry against a sourced LCD pixel aspect before implementing, and keep persistent calculator state deferred behind it.

#### Files Modified:

- README.md
- docs/tiemu-ae350-port.md
- firmware/apps/tiemu_smoke/Makefile
- firmware/apps/tiemu_smoke/main.c
- firmware/common/t89_pad.c
- firmware/common/t89_pad.h
- firmware/include/tiny89_platform.h
- tests/run_t89_pad_test.sh
- tests/test_t89_pad.c
- tools/deploy-ae350-probe.sh

#### Status:

- Build: PASS
- Deployment: PASS
- User Test: PASS

---
