# TiEmu to AE350 port boundary

## Source decision

Tiny89 uses the current public TiEmu history mirror at commit
`2b8df9280d2054e11ffcf7e57a73899b3ce7a7e5`. It explicitly contains TI-89
Titanium memory and hardware-generation support and can synthesize its
emulation flash layout from a TI OS upgrade. Graph89 proves that TiEmu can be
separated from its desktop UI, but its GPLv3 Android wrapper is not used.

Virtual TI was rejected because the published archive is an incomplete
Windows/C++Builder application source set, lacks an explicit Titanium target,
and has unclear redistribution terms. It is not a Tiny89 dependency.

## Split

The host performs all TI file-format handling. `tools/convert_89u.py` validates
the final TIFL record, accepts only a TI-89 Titanium OS payload, constructs the
synthetic boot and hardware-parameter area, and emits a private 4 MiB raw flash
image plus a JSON manifest. The target sees only that fixed-size raw image.

The AE350 runtime retains:

- UAE's generated MC68000 interpreter and opcode tables;
- TI-89 Titanium RAM, flash, memory mapper and protection behavior;
- ASIC I/O, timers, interrupts, keyboard state and LCD addressing; and
- the grayscale/LCD bookkeeping used by TiEmu.

The runtime removes GTK, GDB, sound, D-Bus, Android/JNI, libtifiles,
libticalcs, libticables and libticonv. A small single-threaded compatibility
layer supplies the GLib list operations still embedded in TiEmu's breakpoint
and memory paths. The calculator link backend is stubbed.

## Measured evidence

The host executable contains 620,600 bytes of text, 42,336 bytes of initialized
data and 717,504 bytes of BSS. With the supplied OS it found SSP `0x00004c00`
and PC `0x00812188`, changed the LCD hash 11 times, and settled at PC
`0x00821e46` with LCD FNV-1a `0c0df297` after 20 million interpreted
instructions.

The RV32IMAC ELF contains 4,625,618 bytes of text, including the private 4 MiB
flash image, 34,576 bytes of initialized data and 455,636 bytes of BSS. Its
loadable image is 4,660,196 bytes at `0x40000000`; the entry is `0x40000000`.

On the Tang Console 138K on 2026-10-15, the image transferred through
TinyTang at 186.5 KB/s. The AE350 executed ten million MC68000 instructions
without a TiEmu stop condition, reached PC `0x00821e46`, produced final LCD
hash `0c0df297`, printed `Tiny89 READY`, and exited zero. The user confirmed
that the TI-89 Apps desktop appeared on HDMI.

Later the same day the diagnostic became a continuous session: it runs one
60 Hz frame of emulated time (the TI-89 Titanium's 12 MHz OSC1 divided by 60,
200,000 M68K cycles) per iteration, presents the LCD at the platform's
vertical blank into alternating frame buffers, and never exits, so the
loader's `still running` result is its normal one. Its ELF contained 4,627,200
bytes of text, 34,584 of initialized data and 455,636 of BSS, with SHA-256
`8459e356074a92bde2a7da4837c146be9b4a4d3e494948f619a7dc2cb3068045`. Loaded to
the AE350 it reported the same vectors (`SSP=0x00004c00`, `PC=0x00812188`), the
pad unit answering as `544e5031`, and then 480 presented frames at about one
per vertical blank, reaching the same `0x00821e46` and `0c0df297` desktop state
as the host probe. Its keypad came from a Wii Classic Controller on PMOD0's
first bus through the inherited fabric's registers at `0xe8040000`; the user
pressed the controller and confirmed the mapping works. The mapping's host-test
reports 205 checks passed.

The inherited platform fixed the picture's shape: its video unit shows a
256x240 frame buffer at 3, as many as 960 columns of 1280, so the calculator's
160x100 LCD appeared as a 160x100 patch at 1:1, 480x300 on screen. Tiny89 now
has its own video unit instead: `fpga/tiny89_soc/` is forked from TinyNES at
`f5011dd`, and its `video.sv` scans a 160x100 one-byte-per-pixel image, two
buffers in the SDRAM's top 128 KiB at a 256-byte row stride, as a whole scale
of 7 in both axes -- 1120x700 centred at (80,10) in 1280x720, square pixels,
seven shades of nothing interpolated -- with the app's grayscale levels 0 to 3
read straight out of the unit's four-entry palette. The app finds that unit by
its ID register, `0x54383956` ("T89V"), and keeps a 1-bit path for the
inherited one, so one ELF drives either platform.

That image was built by `tools/build-tiny89-soc.sh` with the open flow: Yosys,
nextpnr-himbaechel and Apicula, no Gowin EDA. It places and routes eight seeds
over one synthesis result and packs the first whose pixel clock has no hold
violations and whose routing passes every gate TinyNES measured for this chip
-- the AE350's dedicated PLL hop, 75 MHz on the bus, 74.25 MHz on the pixel
clock, every video block RAM written and read on one clock and in a 9-bit
mode, and the pixel clock reaching the clock spines only through
`SPINE26 <= CBRIDGEOUT_BOTTOM2`. Seed 1 did: 79.60 MHz on the bus and
86.53 on the pixel clock. The packed image is 4,682,732 bytes, SHA-256
`00ba0bf4e886cdfc7906d4fa6f542e730b043feebfd2e4ba5b495a0354c310c1`, and the
user confirmed the result on HDMI: the calculator's screen fills most of the
720p frame. The session reports `Tiny89 VIDEO T89V`, so the app took its own
unit's path. It also reports `LCDMODE=1bit`: TiEmu's plane model has a single
plane in use on the AMS desktop, so the picture is drawn with two of the four
palette entries, and the four-level path is implemented but not yet
demonstrated on hardware.

`tests/tiny89_video_tb.sv` checks the unit on its own outputs -- 2,764,858
comparisons of every screen pixel against the geometry and palette, the 7x7
mapping, the bars, the buffer flip at a vertical blank, and the ID. One check,
the FRAMES register's rate against the bench's own vertical-blank count, does
not agree and is recorded as unresolved rather than passing.

These sizes and rates describe this diagnostic build; they are not device or
emulator limits.
