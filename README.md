# Tiny89

Tiny89 is a TI-89 Titanium emulator for the Tang Console 138K. The current
milestone runs a headless, dependency-stripped TiEmu 3.04 core on the hardened
AE350 RISC-V CPU and displays the calculator LCD through the existing TinyNES
AE350 HDMI platform.

On 2026-10-15 the supplied TI-89 Titanium OS 3.10 upgrade booted on the Tang,
reached the AMS Apps desktop, produced the same stable LCD hash as the host
probe (`0c0df297`), and exited its ten-million-instruction smoke test without a
CPU exception. The same day the diagnostic became a continuous session -- it
presents the LCD at the platform's vertical blank and runs until the board is
reset -- and took its keypad from a Wii Classic Controller on PMOD0's first
bus, which the user confirmed works.

## Build

The TI operating-system image is proprietary and is never stored in this
repository. Keep it outside the checkout. Build both the private raw image and
the AE350 diagnostic with:

```sh
tools/build-ae350-probe.sh /path/to/TI89Titanium_OS.89u
```

This fetches the pinned TiEmu source, generates its UAE MC68000 tables, creates
`build/private/TI89Titanium_OS.raw`, and links
`build/firmware/apps/tiemu_smoke/tiemu_smoke.elf`. Both `build/` and
`third_party/` are ignored.

The host boot proof can be reproduced with:

```sh
tools/build-host-probe.sh
build/host-probe/tiny89-host-probe build/private/TI89Titanium_OS.raw
```

The host probe succeeds when the emulated LCD changes during boot. The AE350
probe additionally prints boot vectors, program counters, cycle counts, LCD
hashes and controller state over UART, and centers the 160x100 monochrome LCD
on HDMI. The controller mapping has its own host test,
`tests/run_t89_pad_test.sh`, which needs neither board nor toolchain.

Deploying to the board is two steps, because the AE350's boot ROM must be the
thing on the other end of the pipe: load the core image on TinyTang's console
with `tangload /root/cores/console138k/tiny89-ae350.bin`, then run
`tools/deploy-ae350-probe.sh`. The program is continuous, so the loader's own
"still running" result is expected and the script checks for its marker
instead. A power cycle reloads whatever core the FPGA's own flash holds, so
both steps repeat after one.

## Current boundary

The retained runtime contains the UAE MC68000 interpreter and TiEmu's TI-89
Titanium memory, flash, ASIC, keyboard, timer and LCD behavior. GTK, Android,
GDB, sound, calculator-link libraries, GLib itself, and on-target `.89u`
parsing are excluded. Link transfer is stubbed for this milestone.

Input is a Wii Classic Controller mapped to the calculator's keypad in three
layers (see `firmware/common/t89_pad.h`); the mapping is a project choice and
can be changed in the one table in `firmware/common/t89_pad.c`. Input work is
paused at this state.

Next work is a Tiny89-owned AE350 platform build whose video path shows the
calculator's 160x100 LCD filling the 720p output, instead of the 1:1 patch the
inherited TinyNES platform centers in a 256x240 frame buffer. Persistent
calculator state is deferred behind that.

See [THIRD_PARTY.md](THIRD_PARTY.md) for licenses and provenance and
[docs/tiemu-ae350-port.md](docs/tiemu-ae350-port.md) for the measured port
boundary.
