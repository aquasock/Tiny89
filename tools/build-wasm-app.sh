#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-2.0-or-later
#
# Build the browser app: the same TiEmu core the board runs, compiled to
# WebAssembly with emscripten, plus web/t89_web.c as the bridge.
#
#   tools/build-wasm-app.sh                  build web/dist/t89.{js,wasm}
#   tools/build-wasm-app.sh --serve          build, then serve the page on :8080
#
# Needs emscripten.  The script uses ~/emsdk if it is there; set EMSDK to
# point elsewhere.  Everything it produces lands in web/dist/, which is
# generated and not committed: the page itself (web/index.html, web/app.js)
# is, and this script is what turns it into something runnable.
set -euo pipefail

root="$(git rev-parse --show-toplevel)"
emsdk="${EMSDK:-$HOME/emsdk}"
tiemu_root="${TIEMU_ROOT:-$root/third_party/tiemu}"
tiemu_src="$tiemu_root/tiemu/trunk/src"
uae_root="$tiemu_src/core/uae"
out="$root/web/dist"

if [[ ! -f "$emsdk/emsdk_env.sh" ]]; then
    echo "build-wasm-app: no emscripten at $emsdk (set EMSDK)" >&2
    exit 2
fi
# shellcheck disable=SC1091
source "$emsdk/emsdk_env.sh" >/dev/null 2>&1

if [[ ! -d "$tiemu_root/.git" ]]; then
    "$root/tools/fetch-tiemu.sh" "$tiemu_root"
fi
# The one change this project makes to the fetched tree; idempotent, so a tree
# fetched before it existed is fixed here rather than failing at link time.
"$root/tools/patch-tiemu.sh"

# The generated MC68000 tables are the same ones the board's build uses.
make -s -C "$uae_root" gen

mkdir -p "$out"

sources=(
    "$uae_root/cpuemu.c"
    "$uae_root/cpudefs.c"
    "$uae_root/cpustbl.c"
    "$uae_root/readcpu.c"
    "$uae_root/fpp.c"
    "$uae_root/missing.c"
    "$uae_root/xmalloc.c"
    "$uae_root/newcpu.c"
    "$tiemu_src/core/ti_hw/flash.c"
    "$tiemu_src/core/ti_hw/gscales.c"
    "$tiemu_src/core/ti_hw/hw.c"
    "$tiemu_src/core/ti_hw/hwprot.c"
    "$tiemu_src/core/ti_hw/kbd.c"
    "$tiemu_src/core/ti_hw/m68k.c"
    "$tiemu_src/core/ti_hw/mem.c"
    "$tiemu_src/core/ti_hw/mem89.c"
    "$tiemu_src/core/ti_hw/mem89tm.c"
    "$tiemu_src/core/ti_hw/mem92.c"
    "$tiemu_src/core/ti_hw/mem92p.c"
    "$tiemu_src/core/ti_hw/memv2.c"
    "$tiemu_src/core/ti_hw/ports.c"
    "$tiemu_src/core/ti_hw/rtc_hw3.c"
    "$tiemu_src/core/ti_hw/tichars.c"
    "$tiemu_src/core/dbg/bkpts.c"
    "$root/src/port/tiemu_port.c"
    "$root/firmware/common/t89_bios.c"
    "$root/firmware/common/t89_file.c"
    "$root/firmware/common/t89_lcd.c"
    "$root/firmware/common/t89_lcd_gray.c"
    "$root/firmware/common/t89_link.c"
    "$root/firmware/common/t89_proto.c"
    "$root/web/t89_web.c"
)

exports='["_malloc","_free","_t89_boot","_t89_run","_t89_pc","_t89_cycles","_t89_lcd_hash","_t89_lcd_changes","_t89_on","_t89_flash_ptr","_t89_flash_size","_t89_ram_ptr","_t89_ram_size","_t89_lcd_image","_t89_lcd_width","_t89_lcd_height","_t89_lcd_stride","_t89_lcd_mode_text","_t89_install","_t89_last_name","_t89_last_type","_t89_last_size","_t89_error","_t89_state_size","_t89_state_build","_t89_state_restore","_t89_shutdown"]'

emcc -std=gnu11 -O2 -fcommon -ffunction-sections -fdata-sections \
    -DNO_GDB -DNO_SOUND -D__inline__=inline \
    -DSTATFS_NO_ARGS=2 -DSTATBUF_BAVAIL=f_bavail \
    -I"$root/src/port" \
    -I"$tiemu_src" -I"$tiemu_src/core" -I"$tiemu_src/core/uae" \
    -I"$tiemu_src/core/ti_hw" -I"$tiemu_src/core/ti_sw" \
    -I"$tiemu_src/core/dbg" -I"$tiemu_src/misc" \
    -I"$root/firmware/include" -I"$root/firmware/common" \
    -sMODULARIZE=1 -sEXPORT_NAME=createT89 \
    -sENVIRONMENT=web,node \
    -sALLOW_MEMORY_GROWTH=1 \
    -sSTACK_SIZE=2097152 \
    -sEXPORTED_FUNCTIONS="$exports" \
    -sEXPORTED_RUNTIME_METHODS='["ccall","cwrap","HEAPU8","UTF8ToString"]' \
    -o "$out/t89.js" \
    ${WASM_EXTRA:-} \
    "${sources[@]}"

ls -l "$out/t89.js" "$out/t89.wasm"

if [[ "${1:-}" == "--serve" ]]; then
    echo "serving web/ on http://localhost:8080"
    exec python3 -m http.server 8080 --directory "$root/web"
fi
