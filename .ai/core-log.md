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

## 5 COMMIT Unreleased 2026-10-15T09:32:09-07:00

#### Coming From:

Unreleased acc1297

#### Purpose:

Replace the PMOD0 controller with the board's Bluetooth keyboard, so the calculator is driven by a Logitech keyboard that TinyTang forwards to the core, following the mouse path TinyCreature proved.

#### Outcome:

TinyTang gained the core-directed input frame `FPGA_CMD_KEYBOARD 0x17` in `ports/bl616/tang_fpga_link.h` and a sender beside the mouse's in `ports/bl616/tang_osd_desk.c`, carrying the Bluetooth keyboard's HID boot report -- the modifier byte and six usage codes -- sent only when the report changes, because a boot keyboard reports state rather than events; only the Bluetooth keyboard is forwarded, the wired report being one a core has sent and would only be echoed back. That firmware builds to 623,968 bytes with SHA-256 `31a303c2e9fc250b56428db5d5e3827f7e510156b2a238295b9b9f859b3d8533`, was installed with TinyTang's own `tools/tinytang_flash.py` and a power cycle, and the board now reports `62fe6a7-dirty.616fce5` against `6145683-dirty.7381d83` before it; two earlier flash attempts sat unanswered on that tool's interactive `install ... [y/N]` prompt, which is why nothing reached the board and its last command stayed `platform`. In Tiny89, `fpga/tiny89_soc/ae350_soc.sv` no longer instantiates the forked `wii_pads` unit and no longer routes PMOD0, with those pins dropped from `ae350_soc.cst` and the pad window left decoded but answering zero; `desk_link.sv` parses `0x17` into a report split over two registers, `0x1c` as {usage3, usage2, usage1, modifiers} and `0x20` as {sequence, usage6, usage5, usage4}, with the sequence advancing per frame; `exts_uart.sv` reads them. `firmware/common/t89_key.c` maps HID usages to TiEmu keys -- arrows, Enter, Escape and Backspace, Tab for 2nd, F1 to F4 for the diamond, ALPHA, MODE and CATALOG, the digits, the letters and the arithmetic keys including the keypad -- with the modifiers deliberately unmapped, and `firmware/apps/tiemu_smoke/main.c` polls the sequence byte, diffs a report against the one it last applied, and puts the UART in its link mode after printing `TinyNES LINK`, the line TinyNES's loader watches for, because while the host's pipe owns the link no frame can reach the core. The app builds to 4,627,562 bytes of text and the mapping's host test reports `t89_key: 31 checks passed`; the eight-seed platform build passed every gate at seed 8 with no hold violations and no pixel-clock fallbacks, bus 83.97 MHz and pixel 92.32 MHz, and packed a 4,762,092-byte image at SHA-256 `65e4b59a` -- but that image was packed from a build that had started before these sources changed, and it carries neither the keyboard registers nor the pad removal. That is what the deployment shows: the app booted, reported `Tiny89 VIDEO T89V` and `TinyNES LINK`, and then took a load access fault, `BAD Tiny89 TRAP 00000005 4000021e e8000020`, reading exactly the address of the high keyboard register -- a new program on a fabric that has no such register, which both the trap's address and the netlist's missing `kbd` cells agree on. Three faults of this session's own are recorded because each cost a cycle: a build invocation reported as failed that had in fact started and kept running for thirty minutes, so two builds wrote the same output directory while an older one packed stale artifacts; the sweep's concurrency limiter let all eight placements run at once, twice, the second attempt using `jobs -r` inside a command substitution where a subshell has no job table, so both tools now default to one placement at a time; and one edit was made with index arithmetic that duplicated a region of `ae350_soc.sv` instead of deleting it, which was reverted and redone. The user deferred the cycle here for a handoff rather than continue. The required core-syntax audit re-read `.ai/core.md` and `.ai/core-syntax.md`, inspected the complete `.ai/` diff, confirmed that `.ai/core.md` was unchanged, validated this sequential entry's six sections, prose, Status values and the log's five-entry count, and confirmed that no settled entry was rewritten.

#### Next Steps:

A clean platform build was still running when this cycle was deferred, into `build/tiny89_soc_new` rather than the directory the stale images came from, and it has not been deployed or tested. The next step is to deploy that image and run the app: it is the first image built from these sources, so if the app still traps on `0xe8000020` the fault is in this repository's fabric logic rather than in a stale artifact, and a diagnostic that reads `0xe8000020` and the retired pad window at `0xe8040000` -- which the new fabric answers as zero and the old one answered as `544e5031` -- tells the two fabrics apart in one run. TinyTang's `0x17` change is in that repository's tree uncommitted and is already flashed on the board, so the handoff includes it; the pad mapping module `firmware/common/t89_pad.c` remains built and tested but is no longer used by the app, and the video unit's FRAMES testbench disagreement from entry 4 is still open.

#### Files Modified:

- firmware/apps/tiemu_smoke/Makefile
- firmware/apps/tiemu_smoke/main.c
- firmware/common/t89_key.c
- firmware/common/t89_key.h
- firmware/include/tiny89_platform.h
- fpga/tiny89_soc/ae350_soc.cst
- fpga/tiny89_soc/ae350_soc.sv
- fpga/tiny89_soc/desk_link.sv
- fpga/tiny89_soc/exts_uart.sv
- tests/run_t89_key_test.sh
- tests/test_t89_key.c
- tools/build-tiny89-soc.sh
- tools/sweep-tiny89-soc.sh

#### Status:

- Build: PASS
- Deployment: FAIL
- User Test: NOT RUN

---

## 6 COMMIT Unreleased 2026-10-16T05:17:32-07:00

#### Coming From:

Unreleased c0fac6b

#### Purpose:

Finish Tiny89's keyboard: put a fabric with the keyboard registers on the board, and get the Logitech keyboard driving the calculator.

#### Outcome:

The keyboard was already written and host-tested by entry 5; what was wrong was the image on the board. The card's `/root/cores/console138k/tiny89.bin` was 4,762,092 bytes -- entry 5's stale pack, made from a build that raced its own source edits -- so the app's read of the high keyboard register at `0xe8000020` took a load access fault. Nothing in the tree was half-applied: `main` was clean and entry 5's handoff is an honest account. The first obstacle to rebuilding was `tools/sweep-tiny89-soc.sh` itself: its concurrency limiter counted `"${pids[@]:-}"`, which on an EMPTY array expands to one empty element rather than to nothing, so `${#pids[@]}` was 1 with no child running, the loop called `wait -n` on an empty set for ever, and the sweep span at 100% of a core without launching a single placement -- with `SWEEP_JOBS=1`, the default, and therefore on every build. That is what stopped the previous cycle and what I repeated before finding it: my own first two attempts put a second sweep over the first one's directory, which entry 5's own account warns about, and I record it here. The limiter now takes its count from the array itself and leaves it truly empty, and `tests/run_sweep_limiter_test.sh` runs the real script against a stub placer and asserts, for `SWEEP_JOBS=1` and `2`, that every seed is placed and that overlaps never exceed the limit; it fails on the old script exactly as diagnosed (exit 124, 0 of 4 seeds placed) and passes on the fixed one, six checks. With that fixed and `TINY89_SWEEP_JOBS=8` the sweep places all eight seeds in parallel in about eight minutes; the ranking was seed 4 (holds 0, fallbacks 33, bus 85.23, pixel 80.04), seed 6 (10, 22, 88.07, 82.43), seed 8 (10, 27, 83.58, 82.46), seed 3 (10, 31, 91.36, 82.60), seed 1 (10, 58, 83.30, 84.43), seed 5 (10, 62, 88.85, 86.26), seed 7 (10, 95, 84.59, 83.28) and seed 2 (10, 109, 88.70, 84.16); seed 4 is the only placement with no pixel-clock hold violation, which the rule puts first, and it was packed after passing every gate -- the AE350's PLL hop, 85.23 MHz on the bus, 80.04 MHz on the pixel clock, twelve video block RAMs on one clock in a 9-bit mode, and the pixel clock reaching the spines only through SPINE26 <= CBRIDGEOUT_BOTTOM2. The evidence that this is the right image and not another stale one is that my synthesis from the committed tree reproduced the previous build's netlist hash exactly (`4a07c020e89b35ff9eb58c2a33b475e3dfd865509a409fab9b7972163badfbcd`) and that packing it reproduced that build's image bit for bit, so the correct fabric had been on disk all along. It is 4,698,604 bytes with SHA-256 `ec9b3027050c49791110eefd84e8390b8a0b21b7892fec29f0bfcc8025c77131`, put on the card as `/root/cores/console138k/tiny89.bin` with TinyTang's `tangput`, and loaded with `tangload`. The app, `e971e7e9cc262b9bb8fb1439ca4d913bec4df4e729b03061fb8df934cc2b725e`, then booted with no fault at all: `Tiny89 VIDEO T89V`, `Tiny89 KBD seq=0` -- the register that used to fault, read and answered -- `Tiny89 READY`, and a heartbeat whose fields are now `KBDS` and `KEYS` with no `PAD OK` line, because the pad unit is gone. The keyboard then worked: the user reached the AMS home screen and did arithmetic on it. Two things about the path were learned on the way and belong in the record. First, why every watched run showed `KBDS=0`: TinyNES's `load_ae350.py --tinytang` drives the board's `fpgapipe`, which hands the link to the host and holds it for the whole run, so while a PC is reading the program the board's desk layer sends nothing at all -- the same exclusivity entry 79 found for the mouse -- and the working sequence is to load the core, start the program, let the reader go, and then type, with the program still running in the AE350's SDRAM. Second, why a launch costs about a minute: the program is 4,662,148 bytes and goes over the link at 186.5 KB/s, and 4,194,304 of those bytes -- 90 percent -- are the proprietary TI-OS image, which `firmware/apps/tiemu_smoke/Makefile` links into the app through `flash.S` and `flash.raw`, so the whole operating system is re-sent on every launch, and TiEmu then builds its 1866 CPU functions at runtime. That is a defect in how the app was put together rather than a limit of the board, and the fix -- hold the OS in the dock's 32 MiB SDRAM and stop re-sending it, or serve it from the card through the BL616's file server -- is its own cycle. The core-syntax audit re-read `.ai/core.md` and `.ai/core-syntax.md`, inspected the complete `.ai/` diff, confirmed `.ai/core.md` unchanged, and validated this entry's six sections, sequential number, prose, Status values and the log's 100-entry limit.

#### Next Steps:

Stop streaming the operating system into the AE350 on every launch: keep the 4 MiB image in the dock's SDRAM and have the app read it there, or serve it from the card through the BL616, which would take a launch from about a minute to about three seconds. Record the working Tiny89 sequence where a user will find it: `tangload` the core, run the program through the loader, let the loader go, and only then drive it from the keyboard, because the loader owns the link for the whole run. TinyTang now carries an unflashed fix of its own -- the keyboard frame is sent only from `desk_poll()`, which runs while the desk layer is enabled, so a key does nothing when the layer is off -- built and host-tested but not yet on the board; the Tiny89 case works without it because the layer is on there. Entry 4's open items stand: the video unit's FRAMES testbench disagreement, and the four-level grayscale path implemented but not demonstrated, every heartbeat still reading `LCDMODE=1bit`.

#### Files Modified:

- tests/run_sweep_limiter_test.sh
- tools/sweep-tiny89-soc.sh

#### Status:

- Build: PASS
- Deployment: PASS
- User Test: PASS

---
