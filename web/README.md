# The browser image builder

A page that boots the TI-89 Titanium operating system in the same TiEmu core the
board runs, installs the files you choose with the calculator's own software,
and writes out the two images the Tang loads:

- `<name>.flash` — 4 MiB, the calculator's flash, what the runtime asks for with
  `Tiny89 FLASH WANT`
- `<name>.ram` — 256 KiB, the calculator's RAM, what it asks for with
  `Tiny89 NVRAM WANT`

Both go on the card in `/root/saves/ti89/`. Nothing is uploaded anywhere: the
upgrade file, your files and the emulator all live in the tab.

## Build and run

```sh
tools/build-wasm-app.sh          # needs emscripten; writes web/dist/
tools/build-wasm-app.sh --serve  # ... and serves this directory on :8080
```

Then open <http://localhost:8080>, pick the `.89u` TI distributes, pick your
files, and press Build. The page is static; `web/dist/` is generated and not
committed.

## What is checked, and where

- `tests/run_t89_wasm_test.sh` boots the private image in the browser build and
  requires the same desktop the board reaches: PC `0x00821e46`, LCD FNV-1a
  `0x0c0df297`. That is the project's boot invariant (core-log entries 2, 4, 6),
  so a change that breaks the emulator's fidelity fails here rather than on the
  board.
- `tests/run_t89_wasm_test.sh --install <file>` runs the install path end to
  end and reports what the calculator said.
- `tests/run_t89_file_test.sh` covers the container parser the installer uses.

## What works, and what does not

- Variables, programs and data (`0x1c`, `0x12`, `0x21`, …) install as they do
  over a real cable: `t89_proto_send_var()` drives the emulated link port and
  pumps the emulated CPU while it waits for the calculator's answers.
- **Flash Apps (`**TIFL**` containers, type `0x24`) do not install yet.** They
  use a different exchange (`send_app` in libticalcs, not `send_var`), so they
  are recognised and reported rather than sent as the wrong thing.
- The picture is the calculator's own: `t89_lcd_render()` from the board's
  code, four levels, redrawn after every chunk of emulated time.

## How it is put together

`t89_web.c` is the bridge — it converts the upgrade with `t89_bios.c`, starts
TiEmu, seeds the gray-plane model the way the GTK frontend did, and exposes
boot / run / install / export to JavaScript. `app.js` moves bytes in and out and
draws the screen. The emulator sources are the ones `firmware/apps/tiemu_smoke`
and `src/host/boot_probe.c` use; nothing is forked.
