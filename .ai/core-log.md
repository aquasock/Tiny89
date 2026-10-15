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

## 4 COMMIT Unreleased 2026-10-15T08:42:35-07:00

#### Coming From:

Unreleased 8408f67

#### Purpose:

Give Tiny89 its own AE350 platform instead of the inherited TinyNES image, showing the calculator's 160x100 LCD at a whole scale of 7 filling most of the 720p output, with the app's four-level grayscale path selected by the platform's own ID.

#### Outcome:

`fpga/tiny89_soc/` and `firmware/boot/` are forked byte-faithfully from TinyNES at `f5011dd`, with their MIT headers kept and the provenance recorded in THIRD_PARTY.md and `docs/tiny89-soc.md`; the only differences are a fork notice, Tiny89 banner strings, the desk layer's font path (which pointed at `fpga/ae350_soc/` and would not have read in this repository) and a `firmware/boot/Makefile` repointed at five support files forked into `firmware/boot/support/`. Tiny89's own `fpga/tiny89_soc/video.sv` replaces the NES picture with the calculator's 160x100 one-byte-per-pixel image, two buffers in the SDRAM's top 128 KiB at a 256-byte row stride, shown at a whole scale of 7 in both axes as 1120x700 centred at (80,10) in 1280x720 with square pixels and black bars of 80 columns and 10 rows, its palette index the pixel's own value 0 to 3 from a four-entry palette, with the ASPECT register now a documented no-op, the desk layer's overlay kept, and ID 0x54383956 ("T89V") replacing 0x544e5631. In firmware, `firmware/common/t89_lcd.c` and `t89_lcd_gray.c` port TiEmu's plane-exposure composition -- the model its dropped GTK frontend applied in `gui/calc/screen.c`, over the plane state `core/ti_hw/gscales.c` maintains and `core/ti_hw/m68k.c` drives -- into a 160x100 four-level image, leaving `gui/calc/calc.c`'s seeding of `lcd_planes[0]` and `ngc` as the app's job; `firmware/apps/tiemu_smoke/main.c` reads the video unit's ID and takes the grayscale path only for "T89V", keeping the 1-bit 256x240 path so one ELF drives either platform, and tools/build-tiny89-soc.sh builds the platform with the open flow, synthesising once and then placing and routing eight seeds and packing the first whose pixel clock has no hold violations and whose routing passes every gate TinyNES measured: the AE350's dedicated PLL hop, 75 MHz on the bus, 74.25 MHz on the pixel clock, every video block RAM written and read on one clock and in a 9-bit mode, and the pixel clock reaching the clock spines only through SPINE26 <= CBRIDGEOUT_BOTTOM2. That sweep is not optional: the forked fabric itself failed the pixel-clock hold check on five of the same eight seeds, and no seed of the new unit passed at TinyNES's own seed 11. The app builds to 4,627,952 bytes of text with the mapping's 205 checks and the new LCD module's 46 checks passing, and the platform packed at seed 1, bus 79.60 MHz and pixel 86.53 MHz, to a 4,682,732-byte image with SHA-256 00ba0bf4e886cdfc7906d4fa6f542e730b043feebfd2e4ba5b495a0354c310c1. That image was put on the card as `/root/cores/console138k/tiny89.bin` -- the inherited `tiny89-ae350.bin` left in place as a fallback -- loaded with tangload, and the program loaded through it at 186.6 KB/s; the session printed the forked boot ROM's Tiny89 banners, the same boot vectors as before (SSP 0x00004c00, PC 0x00812188), `Tiny89 VIDEO T89V`, `Tiny89 PAD OK ID=544e5031` and `Tiny89 READY`, then 238 heartbeats in which the controller's keys and layers changed as the user pressed it, reaching the same 0x00821e46 and 0c0df297 desktop state as every earlier run. The user confirmed that the picture on HDMI looks perfect. Two things are recorded as unresolved rather than passing: every heartbeat also says `LCDMODE=1bit`, so TiEmu's plane model has one plane in use and the picture is drawn with two of the four palette entries, leaving the four-level path implemented but not demonstrated on hardware; and `tests/tiny89_video_tb.sv`, whose 2,764,858 comparisons of every screen pixel against the geometry, palette, 7x7 mapping, bars, buffer flip and ID all pass, disagrees with the unit on the FRAMES register's rate against the bench's own count of vertical blanks. Three of the harness faults found on the way were the testbench's own -- halfwords offered at even indices the unit indexes consecutively, a bus master that ignored HREADY so palette writes were dropped, and read data sampled a cycle early -- and one claim was withdrawn: the unit's nx reaching 224 is the blanking period, not a defect. The required core-syntax audit re-read `.ai/core.md` and `.ai/core-syntax.md`, inspected the complete `.ai/` diff, confirmed that `.ai/core.md` was unchanged, validated this sequential entry's six sections, prose, Status values and the log's four-entry count, and confirmed that no settled entry was rewritten.

#### Next Steps:

Decide the next cycle; the four-level grayscale needs TiEmu's plane model to be engaged on hardware or the limitation to be accepted as two shades, the testbench's FRAMES disagreement needs resolving against the unit's own vblank count, and TiEmu's ngc 7 and 8 modes still quantise onto a four-entry palette where an eight-entry one would fit them. Persistent calculator state remains deferred by the user's instruction.

#### Files Modified:

- README.md
- THIRD_PARTY.md
- docs/tiemu-ae350-port.md
- docs/tiny89-soc.md
- firmware/apps/tiemu_smoke/Makefile
- firmware/apps/tiemu_smoke/main.c
- firmware/boot/Makefile
- firmware/boot/crt0.S
- firmware/boot/link.ld
- firmware/boot/load.c
- firmware/boot/load.h
- firmware/boot/main.c
- firmware/boot/support/memtest.c
- firmware/boot/support/memtest.h
- firmware/boot/support/tinynes.h
- firmware/boot/support/uart.c
- firmware/boot/support/uart.h
- firmware/common/t89_lcd.c
- firmware/common/t89_lcd.h
- firmware/common/t89_lcd_gray.c
- firmware/common/t89_lcd_gray.h
- firmware/include/tiny89_platform.h
- fpga/tiny89_soc/ae350_soc.cst
- fpga/tiny89_soc/ae350_soc.sdc
- fpga/tiny89_soc/ae350_soc.sv
- fpga/tiny89_soc/ahb_boot_rom.sv
- fpga/tiny89_soc/ahb_ram.sv
- fpga/tiny89_soc/ahb_sdram.sv
- fpga/tiny89_soc/desk_font.hex
- fpga/tiny89_soc/desk_link.sv
- fpga/tiny89_soc/exts_uart.sv
- fpga/tiny89_soc/i2s_out.sv
- fpga/tiny89_soc/video.sv
- fpga/tiny89_soc/wii_pads.sv
- tests/run_t89_lcd_test.sh
- tests/test_t89_lcd.c
- tests/tiny89_video_tb.sv
- tools/build-tiny89-soc.sh
- tools/fs_to_bin.py
- tools/sweep-tiny89-soc.sh

#### Status:

- Build: PASS
- Deployment: PASS
- User Test: PASS

---
