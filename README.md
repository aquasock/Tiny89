# Tiny89

Tiny89 is a TI-89 Titanium emulator for the Tang Console 138K. The current
milestone runs a headless, dependency-stripped TiEmu 3.04 core on the hardened
AE350 RISC-V CPU and displays the calculator LCD through the existing TinyNES
AE350 HDMI platform.

On 2026-10-15 the supplied TI-89 Titanium OS 3.10 upgrade booted on the Tang,
reached the AMS Apps desktop, produced the same stable LCD hash as the host
probe (`0c0df297`), and exited its ten-million-instruction smoke test without a
CPU exception.

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
probe additionally prints boot vectors, program counters, cycle counts and LCD
hashes over UART, and centers the 160x100 monochrome LCD on HDMI.

## Current boundary

The retained runtime contains the UAE MC68000 interpreter and TiEmu's TI-89
Titanium memory, flash, ASIC, keyboard, timer and LCD behavior. GTK, Android,
GDB, sound, calculator-link libraries, GLib itself, and on-target `.89u`
parsing are excluded. Link transfer is stubbed for this milestone.

Next work is persistent calculator state, input mapping, continuous frame
execution and a Tiny89-owned FPGA/platform build instead of the temporary
reuse of TinyNES's AE350 SoC image.

See [THIRD_PARTY.md](THIRD_PARTY.md) for licenses and provenance and
[docs/tiemu-ae350-port.md](docs/tiemu-ae350-port.md) for the measured port
boundary.
