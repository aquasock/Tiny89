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

The inherited platform fixes the picture's shape: its video unit shows a
256x240 frame buffer at 3, as many as 960 columns of 1280, so the calculator's
160x100 LCD appears as a 160x100 patch at 1:1, 480x300 on screen. Filling the
output needs a Tiny89-owned video path, which is the port boundary's next step.

These sizes and rates describe this diagnostic build; they are not device or
emulator limits.
