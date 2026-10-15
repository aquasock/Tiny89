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

These sizes and rates describe this diagnostic build; they are not device or
emulator limits.
