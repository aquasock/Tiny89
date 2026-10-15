#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-2.0-or-later

set -euo pipefail

repo_root="$(git rev-parse --show-toplevel)"
tiemu_root="${TIEMU_ROOT:-${repo_root}/third_party/tiemu}"
output_root="${repo_root}/build/rv32-probe"
toolchain_bin="${RISCV_TOOLCHAIN_BIN:-/home/vash/tools/xpack-riscv-none-elf-gcc-15.2.0-1/bin}"
cc="${toolchain_bin}/riscv-none-elf-gcc"
size_tool="${toolchain_bin}/riscv-none-elf-size"
uae_root="${tiemu_root}/tiemu/trunk/src/core/uae"

if [[ ! -x "${cc}" || ! -x "${size_tool}" ]]; then
    echo "error: RV32 toolchain not found in ${toolchain_bin}" >&2
    exit 1
fi

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
mkdir -p "${output_root}"

common_flags=(
    -march=rv32imac_zicsr_zifencei
    -mabi=ilp32
    -O2
    -ffunction-sections
    -fdata-sections
    -DNO_GDB
    -DNO_SOUND
    -D__inline__=inline
    -DSTATFS_NO_ARGS=2
    -DSTATBUF_BAVAIL=f_bavail
    -I"${uae_root}"
)

"${cc}" "${common_flags[@]}" -c "${uae_root}/cpuemu.c" -o "${output_root}/cpuemu.o"
"${cc}" "${common_flags[@]}" -c "${uae_root}/cpustbl.c" -o "${output_root}/cpustbl.o"

"${size_tool}" -A "${output_root}/cpuemu.o" "${output_root}/cpustbl.o"
echo "RV32 CPU-engine compile probe passed for TiEmu ${actual}"
