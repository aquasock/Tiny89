#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-2.0-or-later

set -euo pipefail

repo_root="$(git rev-parse --show-toplevel)"
tiemu_root="${TIEMU_ROOT:-${repo_root}/third_party/tiemu}"
tiemu_src="${tiemu_root}/tiemu/trunk/src"
uae_root="${tiemu_src}/core/uae"
output_root="${repo_root}/build/host-probe"
cc="${CC:-cc}"

if [[ ! -d "${tiemu_root}/.git" ]]; then
    "${repo_root}/tools/fetch-tiemu.sh" "${tiemu_root}"
fi

expected="2b8df9280d2054e11ffcf7e57a73899b3ce7a7e5"
actual="$(git -C "${tiemu_root}" rev-parse HEAD)"
if [[ "${actual}" != "${expected}" ]]; then
    echo "error: TiEmu checkout is ${actual}; expected ${expected}" >&2
    exit 1
fi

make -C "${uae_root}" gen
mkdir -p "${output_root}/obj"

common_flags=(
    -std=gnu11
    -O2
    -fcommon
    -ffunction-sections
    -fdata-sections
    -DNO_GDB
    -DNO_SOUND
    -D__inline__=inline
    -DSTATFS_NO_ARGS=2
    -DSTATBUF_BAVAIL=f_bavail
    -I"${repo_root}/src/port"
    -I"${tiemu_src}"
    -I"${tiemu_src}/core"
    -I"${tiemu_src}/core/uae"
    -I"${tiemu_src}/core/ti_hw"
    -I"${tiemu_src}/core/ti_sw"
    -I"${tiemu_src}/core/dbg"
    -I"${tiemu_src}/misc"
)

sources=(
    "${uae_root}/cpuemu.c"
    "${uae_root}/cpudefs.c"
    "${uae_root}/cpustbl.c"
    "${uae_root}/readcpu.c"
    "${uae_root}/fpp.c"
    "${uae_root}/missing.c"
    "${uae_root}/xmalloc.c"
    "${uae_root}/newcpu.c"
    "${tiemu_src}/core/ti_hw/flash.c"
    "${tiemu_src}/core/ti_hw/gscales.c"
    "${tiemu_src}/core/ti_hw/hw.c"
    "${tiemu_src}/core/ti_hw/hwprot.c"
    "${tiemu_src}/core/ti_hw/kbd.c"
    "${tiemu_src}/core/ti_hw/m68k.c"
    "${tiemu_src}/core/ti_hw/mem.c"
    "${tiemu_src}/core/ti_hw/mem89.c"
    "${tiemu_src}/core/ti_hw/mem89tm.c"
    "${tiemu_src}/core/ti_hw/mem92.c"
    "${tiemu_src}/core/ti_hw/mem92p.c"
    "${tiemu_src}/core/ti_hw/memv2.c"
    "${tiemu_src}/core/ti_hw/ports.c"
    "${tiemu_src}/core/ti_hw/rtc_hw3.c"
    "${tiemu_src}/core/ti_hw/tichars.c"
    "${tiemu_src}/core/dbg/bkpts.c"
    "${repo_root}/src/port/tiemu_port.c"
    "${repo_root}/src/host/boot_probe.c"
)

objects=()
for source in "${sources[@]}"; do
    relative="${source#${repo_root}/}"
    object_name="$(printf '%s' "${relative}" | tr '/.' '__').o"
    object="${output_root}/obj/${object_name}"
    "${cc}" "${common_flags[@]}" -c "${source}" -o "${object}"
    objects+=("${object}")
done

"${cc}" -Wl,--gc-sections "${objects[@]}" -lm -o "${output_root}/tiny89-host-probe"
size "${output_root}/tiny89-host-probe"
echo "Host boot probe ready: ${output_root}/tiny89-host-probe"
