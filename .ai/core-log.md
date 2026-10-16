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

## 7 COMMIT Unreleased 2026-10-16T05:35:13-07:00

#### Coming From:

Unreleased 84a1f3e

#### Purpose:

Correct the two horizontal arrows in the keyboard mapping, which the user found swapped on the board, and close the test gap that let them be swapped.

#### Outcome:

`firmware/common/t89_key.c`'s binding table named the two horizontal arrows the wrong way round: usage `0x4F` was bound to `TIKEY_LEFT` and `0x50` to `TIKEY_RIGHT`, where the HID usage table has `0x50` as Left Arrow and `0x4F` as Right Arrow. Up (`0x52`) and Down (`0x51`) were already right, which is why only the two horizontal keys misbehaved. The user found it by pressing them, and the two entries now carry the spec's usage. It had survived entry 5's audit and this cycle's review because `tests/test_t89_key.c` checked the table for up and down only -- both horizontal arrows were simply unverified -- so the test now checks all four arrows; it reports 33 checks, and putting the old binding back fails exactly the two new ones, `map: 0x50 is left` and `map: 0x4f is right`. The program was rebuilt with `tools/build-ae350-probe.sh` to 4,627,562 bytes of text, ELF SHA-256 `b69a53012367428480cf2bd8c7f33352f1b63e5447f5e5364c13647d51f9dd37` where the deployed one had been `e971e7e9cc262b9bb8fb1439ca4d913bec4df4e729b03061fb8df934cc2b725e`, and deployed as a program load rather than a firmware flash: `tangload` of the card's `tiny89.bin` and then the ELF over the link. The user validated the mapped keys on the board and reports the keyboard working well. One fault of this session's is recorded because it cost the user a test: `load_ae350.py --tinytang` drives the board's `fpgapipe`, which owns the link for the whole run, and I gave the loader a 120-second window, so it was still holding the wire while the user pressed keys -- which reads as the keyboard having broken, when in fact the desk layer was sending nothing by design. The loader is now stopped as soon as the app reports `Tiny89 READY`, a few seconds into the run, so no dead window is left between the picture coming up and the keyboard working. The core-syntax audit re-read `.ai/core.md` and `.ai/core-syntax.md`, inspected the complete `.ai/` diff, confirmed `.ai/core.md` unchanged, and validated this entry's six sections, sequential number, prose, Status values and the log's 100-entry limit.

#### Next Steps:

The mapping itself is the next cycle, and the gaps are named: the calculator's `ON`, `CLEAR`, `HOME`, `APPS`, `ENTER2`, `SHIFT`, `HAND`, `STO`, `NEGATE`, `EE`, `POWER`, `SIN`, `COS`, `TAN`, `LN`, `THETA`, `NU`, `SPACE` and its own `F1` to `F8` reach it as nothing today. Two of these matter more than the rest: `ON` and `CLEAR` are the keys a calculator user presses most, and the main row cannot produce `PLUS` or `MULTIPLY` at all, because those are Shift+`=` and Shift+`8` and the modifier byte is deliberately unmapped, so the report carries `EQUALS` and `TIKEY_8` instead. Fixing that means either honouring Shift or claiming the free PC keys -- `F5` to `F12`, `Home`, `End`, `Delete`, `Insert`, `Page Up` and `Page Down` -- for the missing calculator keys. Then the launch cost: 4,194,304 of the 4,662,148 bytes sent on every launch are the proprietary OS linked into the program through `flash.S`, which is a defect in how the app was put together rather than a limit of the board, and holding that image in the dock's SDRAM would take a launch from about a minute to about three seconds. TinyTang still carries its unflashed hidden-branch keyboard fix, and entry 4's two open items stand: the video unit's FRAMES testbench disagreement, and the four-level grayscale path implemented but not demonstrated, every heartbeat still reading `LCDMODE=1bit`.

#### Files Modified:

- firmware/common/t89_key.c
- tests/test_t89_key.c

#### Status:

- Build: PASS
- Deployment: PASS
- User Test: PASS

---

## 8 COMMIT Unreleased 2026-10-16T05:59:15-07:00

#### Coming From:

Unreleased adf377a

#### Purpose:

Take the calculator's BIOS off the SD card, as the file the user provides, instead of carrying TI's operating system inside the program.

#### Outcome:

What the user is asked for is the upgrade TI ships -- `**TIFL**`, 1,400,021 bytes -- and not a raw image, which is the better shape as well as the one a person has: it is a third of the 4,194,304 bytes a raw image takes, so a launch moves less of it over the link. The program no longer embeds the operating system at all: `flash.S` and its `flash.raw` rule are out of `firmware/apps/tiemu_smoke/Makefile` and the shim is removed from the tree, so `tools/build-ae350-probe.sh` no longer needs the private file for anything and the program's text fell from 4,627,562 to 436,354 bytes. Its BIOS comes over the link the way a cartridge does: `firmware/apps/tiemu_smoke/main.c` prints `TinyNES WANT <address> <bytes>` and receives `TNR1`, the length and the file in the boot ROM's load protocol, which meant linking `firmware/boot/load.c` and the boot support's `uart.c` into the app; TinyTang's `ae350run <program> <file>` is what answers that line, from the card, which is TinyNES's path and needs no computer beside the board. `firmware/common/t89_bios.c` and its header are the conversion, in C, because the card is where the file lives: the `**TIFL**` record walk to its last record, the checks the Python tool makes (device type 0x98, data type 0x23, a boot vector selecting ROM base 0x80, and the payload fitting), and the 4 MiB layout TiEmu 3.04's `ti68k_convert_tib_to_image` uses -- the boot sector at the front, the marker 0xFEEDBABE, the 24-byte hardware parameter block and the system part at 0x12000. It is deliberately the same arithmetic as `tools/convert_89u.py`, and `tests/run_t89_bios_test.sh` proves it rather than assuming it: the C and the Python tool were run on the real file and their outputs compared byte for byte, and they agree. The test also refuses what should be refused -- garbage, a record longer than the file, an OS record too short for a boot sector, a record that is not a TI-89 image, and a last record that is not an OS -- and, since the operating system is proprietary and lives outside this repository, it says so loudly and runs only the refusals when there is no file to compare against. The other four host tests still pass: `t89_key` 33 checks, `t89_lcd` 46, `t89_pad` 205 and the sweep limiter 6. It was deployed with both files put on the card -- the stripped program at `/root/ae350/tiemu_smoke.elf`, 475,760 bytes, and the upgrade at `/root/media/bios/TI89Titanium_OS.89u` in the folder the user made for it -- and then `tangload /root/cores/console138k/tiny89.bin` followed by `ae350run /root/ae350/tiemu_smoke.elf /root/media/bios/TI89Titanium_OS.89u` on the board's own console. The user reports the calculator on screen, which is the whole claim: the BIOS was read from the card, converted on the board, and booted. `tools/deploy-ae350-probe.sh` gained an argument for the upgrade so the host loader can still answer the request from a PC-side file, and the board's `ae350run` is the path this cycle exercised and the one to document; it hands the link back when the program prints `TinyNES LINK`, which is why the keyboard kept working while the run was being watched, unlike the host's `fpgapipe`. One thing is left unexplained rather than tidied away: while `ae350run` held the console, what reached the host was the heartbeat as a frame header, `AA` and a CRLF once a second, rather than text I could read; the proof that the program had not failed was that every BIOS failure path returns from `main`, which would have ended `ae350run` and given the shell back, and it did not. The core-syntax audit re-read `.ai/core.md` and `.ai/core-syntax.md`, inspected the complete `.ai/` diff, confirmed `.ai/core.md` unchanged, and validated this entry's six sections, sequential number, prose, Status values and the log's 100-entry limit.

#### Next Steps:

Explain the `AA` the console shows while the program runs: the heartbeat reaches the host as a frame header and a newline rather than as text, and while the failure paths are ruled out by the program still running, the reading is not understood. Then the mapping, which is the next cycle the user named: `ON`, `CLEAR`, `HOME`, `APPS`, `STO`, `NEGATE`, `EE`, `POWER`, `SIN`, `COS`, `TAN`, `LN`, `THETA`, `NU`, `SPACE` and the calculator's own `F1` to `F8` reach it as nothing, `ON` and `CLEAR` most of all, and the main row cannot produce `PLUS` or `MULTIPLY` because those are Shift+`=` and Shift+`8` and the modifier byte is deliberately unmapped. A `tiny89.tdsh` that names the program and the BIOS would make the card flow a double-click rather than a typed command. TinyTang still carries its unflashed hidden-branch keyboard fix, and entry 4's two open items stand: the video unit's `FRAMES` testbench disagreement and the four-level grayscale path implemented but not demonstrated.

#### Files Modified:

- firmware/apps/tiemu_smoke/Makefile
- firmware/apps/tiemu_smoke/flash.S
- firmware/apps/tiemu_smoke/main.c
- firmware/common/t89_bios.c
- firmware/common/t89_bios.h
- tests/run_t89_bios_test.sh
- tests/test_t89_bios.c
- tools/build-ae350-probe.sh
- tools/deploy-ae350-probe.sh

#### Status:

- Build: PASS
- Deployment: PASS
- User Test: PASS

---

## 9 COMMIT Unreleased 2026-10-16T06:30:40-07:00

#### Coming From:

Unreleased 37dece7

#### Purpose:

Put the card flow on TinyDesk's desktop as a script the user can right-click and run, so that booting the calculator no longer needs two commands typed at the console.

#### Outcome:

`scripts/tiny89.tdsh` is Tiny89's first card-side script. It checks the three files it needs, tangloads `/root/cores/console138k/tiny89.bin`, runs `ae350run` with `/root/ae350/tiemu_smoke.elf` and `/root/media/bios/TI89Titanium_OS.89u`, and returns the program's status, with every path overridable from the session the way `castlevania.tdsh` allows; the header records the working sequence, its own deploy command, and the failure mode found below. It was built to TinyTang's contract rather than to taste: TinyTang's TDESK-009 has the desktop's icons in the user's Desktop folder and a right-click on a `.tdsh` offer Run, which opens the Terminal and runs `tdsh run "<file>"`, and tinydesk-shell's `docs/SCRIPTING.md` is the language, which is `if`/`else`/`endif` with no `&` and no arguments to a script. The first version carried an `fpga` probe between `tangload` and `ae350run`, copying `boot-cart.tdsh` and `castlevania.tdsh`, and that check can never pass on this core: the desk link sees TinyTang's frames only while the UART's link mode is set, because `fpga/tiny89_soc/exts_uart.sv` routes received bytes to the desk link only when CTRL bit 0 is set, and only the program sets it (`firmware/apps/tiemu_smoke/main.c:318`), never the boot ROM, so a core just tangloaded sits in loader mode and answers nothing. Measured on the board on 2026-10-16: `fpga` reported `core 84 answering on UART1 at 2000000 baud` for the desktop core, whose own program is already running, and `no answer on UART1 at 2000000 baud` for the Tiny89 core immediately after `tangload`. The script therefore returned at that gate and never ran the program, which left the fabric's colour bars on screen -- the diagnostic the video unit draws while its enable bit is clear, `video.sv` choosing the layer, else the bars while not enabled, else the picture -- and the keyboard dead, because the desk layer and the keyboard both need link mode, so no key could reach TinyDesk and F12 could not bring the layer back; that state cannot be left from the keyboard, and the header now says so, with a power cycle as the way out, since nothing here reflashes the FPGA. The gate was removed, because `ae350run` already waits for the boot ROM's own `TinyNES LOAD` and reports when it does not come, which is the check that matters here; the script was re-verified by running it on tinydesk-shell's own host port, put on the card at 4538 bytes with SHA-256 `3f3ce036c961800983cfd97aa9c0dffa4bf8b4d00fb32aa4316073422409f765`, and run on the board, where the calculator booted and the console carried heartbeats reading `VID=T89V`, `VBL` counting, and `KBDS=77` advancing as the user typed, with PC and LCD hash differing from entry 8's desktop values because the user was running calculations on it rather than sitting at the AMS desktop. The user reports running equations on it with the Bluetooth keyboard. The core-syntax audit re-read `.ai/core.md` and `.ai/core-syntax.md`, inspected the complete `.ai/` diff, confirmed that `.ai/core.md` was unchanged, validated this sequential entry's six sections, prose, Status values and the log's nine-entry count, and confirmed that no settled entry was rewritten.

#### Next Steps:

Confirm the desktop path itself: the script that booted the calculator this cycle was started from the console by the agent, so the remaining check is a power cycle, `desktop`, and a right-click Run of `tiny89.tdsh` on the TinyDesk desktop, which reaches the same `tdsh run`. The console stays owned by `ae350run` for as long as the program runs, which is by design rather than a fault, and a power cycle is how it is given back. Entry 8's `AA` is still not explained, though this cycle's capture shows the heartbeats arriving as ordinary text while `ae350run` follows them, which is a lead; the launch still re-sends the program and the BIOS on every run; and entry 4's two open items stand, the video unit's `FRAMES` testbench disagreement and the four-level grayscale path still reporting `LCDMODE=1bit`. The keyboard mapping remains the next cycle the user named and is now the shortest path to a more usable calculator, with `ON`, `CLEAR` and the Shift-modified `PLUS` and `MULTIPLY` the gaps that matter most.

#### Files Modified:

- scripts/tiny89.tdsh

#### Status:

- Build: N/A
- Deployment: PASS
- User Test: PASS

---

## 10 COMMIT Unreleased 2026-10-16T07:08:15-07:00

#### Coming From:

Unreleased bf35605

#### Purpose:

Make the program report the way TinyNES's board build does, one line a minute instead of one a second, and keep the per-second stream behind a diagnostic build.

#### Outcome:

`firmware/apps/tiemu_smoke/main.c`'s `HEARTBEAT_FRAMES` moved from `60u` to `3600u` -- about a minute at the fabric's 60 Hz, and the value TinyNES uses for `NES_REPORT` in its `PLAY=1` build -- so the calculator now sits on the console as quietly as a game does, and the per-second stream that found faults on the NES side became a build option, mirroring TinyNES's own `DIAG=1`: `-DT89_DIAG` selects `60u`, and the Makefile gives that build `build/firmware/apps/tiemu_smoke_diag/` while the default keeps `build/firmware/apps/tiemu_smoke/`. Nothing else changed, and `Tiny89 READY` and `BAD` are untouched, which is what `tools/deploy-ae350-probe.sh` waits for and rejects. A define can compile and still be wrong, so the change was checked at the preprocessor first: the default expands to `presented % 3600u == 0u` and `-DT89_DIAG` to `presented % 60u == 0u`. Both images build, the default at `text 436358` and the DIAG image at `436354`, the four bytes being the `#ifdef` and the pair sitting four above entry 8's recorded `436354` for the deployed program. On the board the quiet image booted as every previous cycle did -- `Tiny89 AE350 C OK`, `Tiny89 SDRAM OK 32M`, `TinyNES LOAD`, `Tiny89 BIOS 1400021 bytes of upgrade`, the same boot vectors `SSP=00004c00 PC=00812188`, `Tiny89 VIDEO T89V`, `TinyNES LINK`, and `Tiny89 READY PC=0095c5e6 LCD=5ed11146` once -- and then produced 106 bytes in a 50.2-second window, one heartbeat at `n=3600` reading `PC=00821e46 CYC=1775609980 LCD=0c0df297 VBL=4440 KBDS=0 KEYS=0 VID=T89V LCDMODE=1bit`, where the build it replaced would have printed about fifty lines in that time. It is deployed as `/root/ae350/tiemu_smoke.elf`, 475768 bytes, SHA-256 `b6d69d34ec31fbcd481e2affadef396c0ad1d7d5fd7d14f28da27bed70d90876`. Two faults of this cycle's own are recorded. The first build failed outright, `No rule to make target app_defs`, because the guard that stops a stale `main.c.o` from surviving a define change wrote its options file only when the values differed, so the default build, which defines nothing, wrote no file at all while `main.c.o` depended on one; the test written to check that guard asserted the stamp's contents and never its existence, which is why the flaw reached the real build, and the guard now creates the file when it is missing and rewrites it only when the values change, with TinyNES's Makefile carrying the same hole around its `PLAY` and `DIAG` flags. The second was the artifact itself: the first deployment was a `--strip-debug` build at 567388 bytes, and entry 8's recorded 475760 showed that `--strip-all` is what this card has been given, which reproduces that size to within the four bytes above. That heartbeat also closed entry 9's open question about the LCD hash: a fresh boot reports `PC=00821e46 LCD=0c0df297`, entry 8's own pair, so the `336b6239` and `7965f149` seen at that address earlier were the user's session on the screen rather than a defect. The user compared the result against TinyNES on the board and reports it works the same. The core-syntax audit re-read `.ai/core.md` and `.ai/core-syntax.md`, inspected the complete `.ai/` diff, confirmed that `.ai/core.md` was unchanged, validated this sequential entry's six sections, prose, Status values and the log's ten-entry count, and confirmed that no settled entry was rewritten.

#### Next Steps:

The `DIAG=1` image builds but was never run on the board, so a cycle that needs the per-second stream should exercise it before relying on it. Entry 9's ceiling stands and is now the sharpest limit on the card flow: with the calculator running the BL616 heap reports 25060 bytes free of 120680 and refuses a 16384-byte script worker stack, so no desktop script can start beside a running calculator, and the order that works is the script starting the calculator rather than the reverse. Quieting the heartbeat makes the console usable while the calculator runs, which is what made entry 9's measurements awkward, though the desk layer's own traffic and the keyboard frames still cross that wire, and the calculator still outlives the desktop core it was started from. The keyboard mapping remains the next cycle the user named, with `ON`, `CLEAR` and the Shift-modified `PLUS` and `MULTIPLY` the gaps that matter most. Entry 4's two open items stand, the video unit's `FRAMES` testbench disagreement and the four-level grayscale path still reporting `LCDMODE=1bit` on every heartbeat, and the launch still re-sends the program and the BIOS on every run.

#### Files Modified:

- firmware/apps/tiemu_smoke/Makefile
- firmware/apps/tiemu_smoke/main.c

#### Status:

- Build: PASS
- Deployment: PASS
- User Test: PASS

---

## 11 COMMIT Unreleased 2026-10-16T07:32:59-07:00

#### Coming From:

Unreleased 42338a7

#### Purpose:

Finish the keyboard mapping, so the calculator's own keys -- the arithmetic the main row could not reach, the functions, and the menu and editing keys -- are all reachable from the paired Bluetooth keyboard.

#### Outcome:

`firmware/common/t89_key.c` now holds two tables instead of one. The plain table gained the calculator's own keys, on the PC keys that had no other job here: `Space` to `SPACE`, `Caps Lock` to `HAND`, `Insert` to `APPS`, `Home` to `HOME`, `PageUp` to `STORE`, `Delete` to `CLEAR`, `End` to `ON`, `PageDown` to `EE`, `F6`, `F7` and `F8` to `SIN`, `COS` and `TAN`, and `F9`, `F10` and `F11` to `LN`, `NU` and `THETA`; keypad `Enter` moved from `ENTER1` to `ENTER2`, which is the one binding whose meaning changed rather than being added. A second table, `t89_shift_binds`, holds what Shift turns a key into, and `implied()` now reads the report's modifier byte for the shift bits (`0x02` and `0x20`, the low bits of usages `0xE1` LeftShift and `0xE5` RightShift) and consults it first, falling through to the plain table so Shift with an arrow or a letter is still that key. The four shifted keys are the arithmetic the main row could not reach, and each is the character the standard's own name for the key says it produces: `Shift+=` is `PLUS` (`0x2E` is `= and +`), `Shift+8` is `MULTIPLY` (`0x25` is `8 and *`), `Shift+6` is `POWER` and `Shift+-` is `NEGATE`; `Shift+F1` to `Shift+F8` are the calculator's own soft keys, which leaves `F1` to `F4` free to stay `DIAMOND`, `ALPHA`, `MODE` and `CATALOG`. `firmware/common/t89_key.h` declares the second lookup and its comment no longer claims the modifiers are unmapped. Every calculator key now has a home except `SHIFT`, deliberately: a boot report keeps Shift in the modifier byte rather than in its six usage slots, and that bit is what chooses between the tables, so forwarding it as a key as well would press the calculator's shift on every shifted key. The mapping was written against a source rather than recall. HID Usage Tables for USB 1.4 section 10 was fetched from usb.org, every usage already in use was checked against it -- which confirmed the two horizontal arrows entry 7 corrected and found no other binding wrong -- and it is now recorded as `KEY-001` in `.ai/core-reference.md`, with an index line and a routing row, marked SOURCED rather than VERIFIED because the codes added this cycle had not been confirmed on hardware when the record was written. The host test grew from 33 checks to 134: every binding, plain and shifted, now carries the usage's name from that section, which is the gap that let the arrows be swapped for two cycles, and the shift layer has its own checks for both Shift bits, for falling through, and for a release finding the key's ordinary meaning again. The test was proved to have teeth rather than merely to pass: with `F6`'s usage and the shifted-`8` binding deliberately broken it returned exactly `FAIL map: 0x3f is F6`, `FAIL map: 0x40 is F7` and `FAIL shift map: 0x25 is 8 and * held`, and restoring the file returned 134 passing. The program builds to 436704 bytes of text, 346 more than entry 10's, and the deployed artifact is 476112 bytes at SHA-256 `251968ffa96114cbdaf1ddcc5980f1cabd0c88d3626be052b5c22d26715136e0`, put on the card with TinyTang's `tinytang_put.py` at `/root/ae350/tiemu_smoke.elf` and read back there. On the board it boots exactly as entry 10's image did -- `SSP=00004c00 PC=00812188`, `Tiny89 VIDEO T89V`, `TinyNES LINK`, `Tiny89 READY PC=0095c5e6 LCD=5ed11146` -- and stays quiet. The user tested the mapping on the board through the Bluetooth keyboard and reports it passing. Three smaller things are recorded. Two bindings keep meanings that no PC key justifies -- `;` reaches `COMMA` a second time and the apostrophe key reaches `PIPE`, whose own key is Shift with the backslash -- and the user chose to keep both as they are. My own checks of the mapping were wrong twice before they were right, first a regular expression too narrow to read every usage name and then one that could not match the apostrophe key's escaped quote, and both were corrected before the source and test tables were confirmed to agree on all 89 bindings; the code was never at fault. The core-syntax audit re-read `.ai/core.md` and `.ai/core-syntax.md`, inspected the complete `.ai/` diff, confirmed that `.ai/core.md` was unchanged, validated this sequential entry's six sections, prose, Status values and the log's eleven-entry count, and confirmed that no settled entry was rewritten.

#### Next Steps:

The next cycle the user named is persistent calculator state, which entries 4 and 6 deferred by instruction and which is the last thing between this emulator and a calculator that keeps its memory. The path already exists on the other side: TinyTang's `ae350run` implements TinyNES's `TinyNES NVRAM WANT` and `TinyNES NVRAM SAVE` text protocols and keeps a cartridge's save under `/root/saves/nes/`, so a Tiny89 program that asked for the calculator's RAM at start-up and handed it back at the end would need agreement on where that file lives on the card rather than new machinery on the board. `firmware/apps/tiemu_smoke/main.c` seeds `img_infos` from the converted image and never writes anything back, so the RAM the calculator believes it has is the emulator's own and is lost at every reset. `KEY-001` stays SOURCED until the codes this cycle added are confirmed on hardware; the two odd bindings above remain the user's to move or keep; the `DIAG=1` image still builds without having been run on the board; the BL616 heap still refuses a 16384-byte script worker stack while the calculator runs; and entry 4's two open items stand, the video unit's `FRAMES` testbench disagreement and the four-level grayscale path still reporting `LCDMODE=1bit` on every heartbeat.

#### Files Modified:

- firmware/common/t89_key.c
- firmware/common/t89_key.h
- tests/test_t89_key.c

#### Status:

- Build: PASS
- Deployment: PASS
- User Test: PASS

---

## 12 COMMIT Unreleased 2026-10-16T08:02:17-07:00

#### Coming From:

Unreleased 699b3e2

#### Purpose:

Keep the calculator's RAM across a reset, by handing it to the board on a key press and asking for it back at start-up.

#### Outcome:

Tiny89 has the transport for persistent memory and it works; the calculator does not yet keep its session, and the reason is in the flash. What was built: `firmware/common/t89_save.c` and its header hold the CRC-32 ae350run checks a save with (the reflected CRC-32 of CRC-32/ISO-HDLC, which is zlib's and Ethernet's), the hex a line goes out as and the blank test that tells an unwritten save from a real one; `firmware/common/t89_key.c` gives Control to the program rather than the calculator, so `t89_kbd_command()` names a save and nothing is mapped from a report that carries it, which is why the chord cannot also type; and `firmware/apps/tiemu_smoke/main.c` asks for the RAM at start-up with `TinyNES NVRAM WANT` while the UART is still in loader mode, and hands it back on `Control` with `s` in TinyNES's own protocol, including the three `NVRAM BEGIN` warm-up lines and a four-millisecond pause between lines. The board refused any save over 8192 bytes -- a NES battery's size -- so `AE350_SAVE_MAX` in TinyTang's `ports/bl616/tang_fpga_uart.c` moved to 8 MiB, with TinyTang's own project control left as that repository's business. The host tests grew: `tests/test_t89_save.c` and `tests/run_t89_save_test.sh` are new and report 21 checks, including CRC fixtures taken from zlib rather than from this module's own arithmetic, and `tests/test_t89_key.c` went from 134 checks to 146 for the Control namespace. The program builds to 437554 bytes of text and the deployed artifact is 476960 bytes at SHA-256 `0813516bebbf443bff6296293ef6ac6eeff36fbc85b0e2d595117773e9a1a899`. TinyTang's BL616 firmware was rebuilt from its working tree and flashed; the board's build identity moved from `78a0748-dirty.5428ee7` to `162ff40-dirty.838a81f`, which also carried the uncommitted desk-layer keyboard fix that repository was holding. On the board, both halves of the transport were then verified, and separately. The load was tested first against a file this cycle put on the card itself: 262144 zero bytes at the save's path, whose CRC-32 zlib gives as `e20eea22`, and the running program reported `Tiny89 RAM GOT 262144 e20eea22` after the board served the file and the loader wrote it into `tihw.ram`, byte-exact and with the boot unchanged around it. The save was then the user's to trigger, and it reported `9ca4fec2` over the calculator's live RAM; a power cycle later the same program, loading what the board had written, reported `Tiny89 RAM GOT 262144 9ca4fec2`. That is an independent CRC at each end of a round trip through the card, and it proves the transport: the bytes leave the emulator, are decoded to a binary file by ae350run, survive a reprogramming of the FPGA, come back and land in `tihw.ram` unchanged. It does not prove that the calculator keeps its memory, and it does not: the user reports that after the reset the calculator ran the installation step it performs on a first boot and did not resume the session, so the OS treated the restored RAM as a fresh device's. The lead, and it is a lead rather than a finding: the flash is rebuilt from the converted image at every start, `core/ti_hw/mem.c`'s `memcpy(tihw.rom, img->data, img->size)` followed by `free(img->data)`, so the calculator is handed a virgin flash every boot, with no file system and no record of the RAM having been valid. On a real calculator both the RAM and the flash persist, and the OS decides from what it finds; here only the RAM can persist today, which fits a first-boot installation on every start regardless of the RAM. What this cycle did not establish is whether the OS also overwrites the restored RAM during its boot, which the next cycle has to answer before the flash work is designed, because a RAM restore can never show while the flash is virgin and a flash restore would be wasted if the RAM is overwritten anyway. The core-syntax audit re-read `.ai/core.md` and `.ai/core-syntax.md`, inspected the complete `.ai/` diff, confirmed that `.ai/core.md` was unchanged, validated this sequential entry's six sections, prose, Status values and the log's twelve-entry count, and confirmed that no settled entry was rewritten.

#### Next Steps:

Find out what the calculator does to the RAM before designing the flash half. A diagnostic build of the same program should print a CRC of `tihw.ram` and of `tihw.rom` at both ends of the OS's boot -- once as soon as the save has been loaded and before the CPU first runs, and again when `Tiny89 READY` is printed -- because that distinguishes the two cases that matter: a RAM CRC that changes means the OS re-initialises it whatever we restore, and a RAM CRC that holds while the flash CRC moves means the flash is the missing half and the RAM restore is already right. Then the flash, which is 4 MiB and cannot ride the link as hex in any tolerable time: either the save direction gains a way to carry bytes, which the load direction already has, or the flash is compared against the image it booted from and sent only when it differs. Neither is in this cycle and both want the diagnostic's answer first. The transport built here stays either way, since it is the same machinery the flash needs. Entry 11's open items stand: KEY-001 is SOURCED until the codes that cycle added are confirmed on hardware, the DIAG=1 image still builds without having been run, the two bindings no PC key justifies are still the user's to move or keep, and the BL616 heap still refuses a 16384-byte script worker stack while the calculator runs. Entry 4's two items also stand, the video unit's FRAMES testbench disagreement and the four-level grayscale path still reporting LCDMODE=1bit on every heartbeat.

#### Files Modified:

- firmware/apps/tiemu_smoke/Makefile
- firmware/apps/tiemu_smoke/main.c
- firmware/common/t89_key.c
- firmware/common/t89_key.h
- firmware/common/t89_save.c
- firmware/common/t89_save.h
- tests/run_t89_save_test.sh
- tests/test_t89_key.c
- tests/test_t89_save.c

#### Status:

- Build: PASS
- Deployment: PASS
- User Test: FAIL

---

## 13 COMMIT Unreleased 2026-10-16T08:08:12-07:00

#### Coming From:

Unreleased 9478a08

#### Purpose:

Find out what the calculator's own boot does to the RAM and the flash, because a restored RAM did not resume a session.

#### Outcome:

The answer is the flash, and it was measured rather than inferred. `firmware/apps/tiemu_smoke/main.c` gained a `diag_line()` that prints a CRC-32 of `tihw.ram` and of `tihw.rom`, called once as soon as the save has been loaded and before the CPU first runs, again at `Tiny89 READY`, and on every heartbeat -- all of it under entry 10's `T89_DIAG`, so the default image is untouched, which the build proves: the default is `text 437554` with no `Tiny89 DIAG` string in it and a stripped SHA-256 identical to entry 12's artifact, while the DIAG image is `437720` with the string present. This is also the first time the `DIAG=1` image has run on the board, which entry 10 left open. The readings, on a board running the diagnostic build with entry 12's save still on the card: `BEFORE RAM=9ca4fec2 FLASH=d9109983` -- the restored RAM hashed exactly as the save and its load both did, a third independent confirmation of that transport, and the flash was the converted image as built. Then `READY RAM=0f5d7faa FLASH=d2d6a57b`, both already different within 300 presented frames, about five seconds of the calculator's own time. The flash then moved three more times, `d27ea422`, `0d5fe1d9`, `06d1985b`, and settled at `d1fcdbf9` by the fourth heartbeat, holding that value for the remaining fifty-eight readings of the run. What that establishes is that the OS writes the flash on every boot and finishes doing so within a few seconds: the calculator's file system is re-made each time because the flash it is handed is always the converter's untouched image, which `core/ti_hw/mem.c` copies in at every start, and that is why the user saw the first-boot installation and not the session. What it does not establish is whether the OS would also re-initialise a valid RAM, and the readings cannot say: a running OS writes its RAM continuously, so a changed RAM CRC is expected whether it kept what we restored or replaced it, and the RAM CRC did change on every reading. Settling that needs a flash the OS accepts, which is the next cycle's subject; the honest reading of this run is that the flash is the blocker and the RAM question is still open. One thing is worth recording as a consequence rather than a fault: because the OS's flash writes settle within a few seconds of the boot and then stop, the flash is stable by the time a session is running, so a save taken when the user asks is a save of settled state. The card was left holding the diagnostic image rather than the quiet one, because the run this cycle measured was still holding the console when the attempt was made to put it back -- the same held-console state entry 12 hit, and no failure of anything. The core-syntax audit re-read `.ai/core.md` and `.ai/core-syntax.md`, inspected the complete `.ai/` diff, confirmed that `.ai/core.md` was unchanged, validated this sequential entry's six sections, prose, Status values and the log's thirteen-entry count, and confirmed that no settled entry was rewritten.

#### Next Steps:

Persist the flash. It is `tihw.rom`, 4 MiB, and the shape of the work is now known: the app must write it before the OS's boot code first runs, which is where `core/ti_hw/mem.c` puts the converted image today, and read it back the same way the RAM is read back, with the converted image remaining the fallback for a first boot. The cost is the open problem: 4 MiB as hex at the four-milliseconds-a-line pacing the RAM save uses is about eight minutes, which is not a thing to ask of a user, so either TinyTang's save direction gains a way to carry bytes the way its load direction already does, or the flash is compared against the image it booted from -- a 4 MiB CRC the app can compute -- and sent only when it differs, which for an install-once calculator would be rare and, when it happened, still eight minutes. The RAM question from this cycle comes after that: with a flash the OS accepts, whether it keeps a restored RAM becomes measurable, and only then is it worth deciding anything about the RAM path. The converter's own limits are worth revisiting in the same cycle, since the image it builds is now a fallback rather than the only thing the calculator ever sees. Entry 12's transport stays as it is and is the machinery the flash needs. Entry 11's open items stand: KEY-001 is SOURCED until the codes that cycle added are confirmed on hardware, the two bindings no PC key justifies are the user's to move or keep, and the BL616 heap still refuses a 16384-byte script worker stack while the calculator runs. Entry 4's two items also stand, the video unit's FRAMES testbench disagreement and the four-level grayscale path still reporting LCDMODE=1bit.

#### Files Modified:

- firmware/apps/tiemu_smoke/main.c

#### Status:

- Build: PASS
- Deployment: PASS
- User Test: N/A

---

## 14 COMMIT Unreleased 2026-10-16T12:05:25-07:00

#### Coming From:

Unreleased 48346eb

#### Purpose:

Make the calculator's RAM survive a power cycle in the user's own configuration, where entry 12's transport had worked on the bench and had failed on every attempt the user made.

#### Outcome:

The RAM now round-trips, and the failure entry 12 left unexplained was measured rather than reasoned about. Two changes carry it: `save_ram()` sends four empty paced lines between the header and the payload, and `save_pace()` reads `T89_SAVE_PACE_MS` from `t89_save.h` instead of a hardcoded four milliseconds, that macro raised to eight. The justification is a probe build that saved the first 8192 bytes instead of the whole RAM, because 8192 is what this board's own cartridge saves use: it reported `TinyNES NVRAM SAVE 8192`, then `ae350run: the save did not arrive whole: 8096 of 8192 bytes, 0 dropped in the link ring, close 0` -- exactly three lines short, with nothing counted as lost at either end, so bytes were being dropped by something that does not report it, and the payload's opening was where the project's own comments already said a burst loses a chunk. An empty line decodes to nothing, so the warm-up spends that loss on air. With the warm-up and the pace together the same probe saved whole and the file was served back, and then the full RAM did the same: `TinyNES NVRAM SAVE 262144 1f7fd361`, `ae350run: saved 262144 bytes to /sd/root/saves/nes/TI89Titanium_OS.sav`, `Tiny89 RAM SENT 262144 1f7fd361 65832 ms`, and `ls -l` showed 262144 bytes on the card. After a power cycle the board answered `ae350run: sent /sd/root/saves/nes/TI89Titanium_OS.sav, 262144 bytes` and the app computed `Tiny89 RAM GOT 262144 1f7fd361` over the loaded RAM -- the same CRC the save carried, from an independent computation. That closes the question entry 12 opened: every full-size save before this one came back complete with a disagreeing CRC, at both encodings and both paces, and the mechanism was written down as not understood. It was never about size or encoding; it was frames going missing uncounted. Three hypotheses were tested and rejected along the way: that the console port collision between a tool holding the port and the program printing on the same wire was the cause, which the counter of keyboard frames reaching the app falsified; that stale firmware explained the input failure, which the build identity did not support; and that three lines of loss needed a size-dependent explanation, which the 8192-byte probe disproved. Getting the save this far took a second fault out of the way first: the calculator ignored its keyboard entirely, and the cause was in TinyTang rather than this repository -- its desk layer forwarded the keyboard to a core only while the desktop layer was drawn, so a key did nothing while anything else held the screen. That fix was already written but uncommitted in TinyTang, alongside this cycle's own retry fix there, which stops `keyboard_frame()` recording a report as sent before the link has accepted it, so a poll whose lock was refused no longer loses the keystroke for good. The board now runs a firmware built from those changes, `162ff40-dirty.627b0e9`, flashed with `tangflash` and a power cycle, and `platform` confirms it. The app deployed is the stripped ELF at `/root/ae350/tiemu_smoke.elf`, `477056` bytes as the card holds it and `472324` as ae350run's loader reads it, SHA-256 `5a3269f591990390a4458a3c552475df73db9f7cc60cab2d3999b566cfd88e03`, and the default build reports `text 437650`; both host tests pass, `t89_save` at 21 checks and `t89_key` at 146. `tests/test_t89_save.c` carries a header comment listing what it covers and one sentence of it had been dropped in the working tree; it was put back to the text the repository already held, so this cycle changes no test file and the coverage above is unchanged. The user reported the keyboard working -- the cursor moved on the Apps desktop -- and directed the cycle closed. What this cycle does not do is make a session resume: the flash is still rebuilt from the converted `.89u` at every start by `core/ti_hw/mem.c`, so the OS wakes to a virgin file system, which is entry 13's finding and unchanged. Whether the OS keeps a restored RAM is therefore still open, and the user's own test -- save a variable, Ctrl+S, reboot, look for it -- is the measurement that would settle it once a flash the OS accepts exists. One limitation worth recording: the full-RAM save takes 65.8 seconds and freezes the picture for that time, which is the price of the eight-millisecond pace, and that pace is the one number a later cycle can revisit now that the loss is understood. The core-syntax audit re-read `.ai/core.md` and `.ai/core-syntax.md`, inspected the complete `.ai/` diff, confirmed that `.ai/core.md` was unchanged and that no settled entry was rewritten, and validated this entry's sequential number, six sections, prose, Status values and the log's fourteen-entry count against the template.

#### Next Steps:

Persist the flash, which is `tihw.rom` at 4 MiB and entry 13's subject. The shape is known -- the app must put the flash in place before the OS's boot code first runs, and read it back the way the RAM is read back, with the converted image remaining the fallback for a first boot -- but the cost is the open problem: 4 MiB as hex at eight milliseconds a line is about nine minutes, which is not a thing to ask of a user. This cycle changes the arithmetic of that decision, because TinyTang's save direction already carries a binary payload and the app's binary payload was abandoned only because of a CRC disagreement that this cycle has now traced to uncounted frame loss; retrying the binary shape with the payload warm-up and the pace in place is the cheapest thing to measure first, and if it holds it makes the flash transfer a matter of seconds. The RAM question comes after that: with a flash the OS accepts, whether it keeps a restored RAM becomes measurable, and only then is it worth deciding anything about the RAM path. The TinyTang changes this cycle depends on are in neither repository's log yet -- the desk layer's hidden-branch keyboard forwarding and the accepted-before-sent retry -- and the board runs a firmware built from them, so that repository's own cycle and log need to catch up before the next firmware build. Entry 11's open items stand: KEY-001 is SOURCED until the codes that cycle added are confirmed on hardware, the two bindings no PC key justifies are the user's to move or keep, and the BL616 heap still refuses a 16384-byte script worker stack while the calculator runs. Entry 4's two items also stand, the video unit's FRAMES testbench disagreement and the four-level grayscale path still reporting LCDMODE=1bit.

#### Files Modified:

- firmware/apps/tiemu_smoke/main.c
- firmware/common/t89_save.h

#### Status:

- Build: PASS
- Deployment: PASS
- User Test: PASS

---
