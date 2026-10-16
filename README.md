# Tiny89

Tiny89 is a TI-89 Titanium emulator for the Tang Console 138K. The current
milestone runs a headless, dependency-stripped TiEmu 3.04 core on the hardened
AE350 RISC-V CPU, on Tiny89's own AE350 platform image, and shows the
calculator's LCD at a whole scale of 7, filling most of the 720p frame.

On 2026-10-15 the supplied TI-89 Titanium OS 3.10 upgrade booted on the Tang,
reached the AMS Apps desktop, produced the same stable LCD hash as the host
probe (`0c0df297`), and exited its ten-million-instruction smoke test without a
CPU exception. The same day the diagnostic became a continuous session -- it
presents the LCD at the platform's vertical blank and runs until the board is
reset -- and took its keypad from a Wii Classic Controller on PMOD0's first
bus, which the user confirmed works. Tiny89 then built its own platform image
from forked sources and the user confirmed the picture: the calculator's
160x100 LCD at a whole scale of 7, 1120x700 centred in 1280x720 with square
pixels. The four-level grayscale path is in the app, but on the AMS desktop
TiEmu's plane model reports a single plane, so the session draws with two of
the four shades today.

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

Deploying to the board is three steps, because the AE350's boot ROM must be the
thing on the other end of the pipe. Build the platform image first:

```sh
tools/build-tiny89-soc.sh          # synthesise, sweep 8 placements, pack the first that passes
```

It places and routes eight seeds over one synthesis result, keeps only a seed
whose pixel clock has no hold violations and whose picture passes every
measured gate, then packs that one to `build/tiny89_soc/tiny89_soc.bin`. Then
put the image on the card and load it as the FPGA core on TinyTang's console:

```sh
python3 "$TINYTANG_DIR/tools/tinytang_put.py" --port /dev/ttyACM0 \
    build/tiny89_soc/tiny89_soc.bin /root/cores/console138k/tiny89.bin
# on the board:  tangload /root/cores/console138k/tiny89.bin
```

and finally load the program with `tools/deploy-ae350-probe.sh`. The program is
continuous, so the loader's own "still running" result is expected and the
script checks for its marker instead. A power cycle reloads whatever core the
FPGA's own flash holds, so all three steps repeat after one.

## Current boundary

The retained runtime contains the UAE MC68000 interpreter and TiEmu's TI-89
Titanium memory, flash, ASIC, keyboard, timer and LCD behavior. GTK, Android,
GDB, sound, calculator-link libraries, GLib itself, and on-target `.89u`
parsing are excluded. Link transfer is stubbed for this milestone.

Input is a Wii Classic Controller mapped to the calculator's keypad in three
layers (see `firmware/common/t89_pad.h`); the mapping is a project choice and
can be changed in the one table in `firmware/common/t89_pad.c`. Input work is
paused at this state.

The calculator's RAM is saved with Ctrl+S, which is the program's own key
rather than the calculator's, and reaches the card over TinyTang's NVRAM
protocol from `firmware/common/t89_save.h`. The board verifies what it writes:
`ae350run` says `saved ... read back and verified` only after reopening the
file and hashing it, and a save that does not verify leaves the one already on
the card untouched, so pressing the key again costs the time and nothing else.
Two habits follow from that. Read the board's own line before believing a save
-- only the verified one means the card holds it. And check the core is still
active afterwards: the heartbeat is the instrument, and it advances only while
the program is presenting frames. The calculator powers itself down after a
few idle minutes and that state is indistinguishable from a dead keyboard --
the picture stops, no key does anything -- and End is its ON key.

The video path is Tiny89's own (`fpga/tiny89_soc/video.sv`), and the app picks
it by reading the video unit's ID register, so the same ELF still runs on the
inherited TinyNES platform. `tests/tiny89_video_tb.sv` checks the geometry, the
four levels and the buffer flip on the unit's own outputs; one of its checks,
the FRAMES rate against the bench's own vertical-blank count, does not agree
yet and is unresolved rather than passing.

Known gaps: the four-level grayscale is implemented but not demonstrated on
hardware -- TiEmu's plane model reports one plane (`ngc = 1`) on the AMS
desktop, so the picture uses two shades and the four-entry palette is only
partly exercised; TiEmu's model composes up to eight exposures for its `ngc` 7
and 8 modes, which the four-entry palette would quantise; and the testbench
covers neither the TMDS encoding nor the desk layer's overlay, both inherited
and unchanged.

Next work is to be decided; persistent calculator state remains deferred.

See [THIRD_PARTY.md](THIRD_PARTY.md) for licenses and provenance and
[docs/tiemu-ae350-port.md](docs/tiemu-ae350-port.md) for the measured port
boundary.
