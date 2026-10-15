#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-2.0-or-later

set -euo pipefail

repo_root="$(git rev-parse --show-toplevel)"
os_upgrade="${1:-/run/media/vash/GIT/TI89Titanium_OS.89u}"
tiemu_root="${TIEMU_ROOT:-${repo_root}/third_party/tiemu}"
raw_image="${repo_root}/build/private/TI89Titanium_OS.raw"
uae_root="${tiemu_root}/tiemu/trunk/src/core/uae"

if [[ ! -f "${os_upgrade}" ]]; then
    echo "error: OS upgrade not found: ${os_upgrade}" >&2
    exit 1
fi

"${repo_root}/tools/fetch-tiemu.sh" "${tiemu_root}"
make -C "${uae_root}" gen
"${repo_root}/tools/convert_89u.py" "${os_upgrade}" "${raw_image}"

make -C "${repo_root}/firmware/apps/tiemu_smoke" \
    OS_IMAGE="${raw_image}" \
    TIEMU_ROOT="${tiemu_root}"

elf="${repo_root}/build/firmware/apps/tiemu_smoke/tiemu_smoke.elf"
echo "AE350 diagnostic ready: ${elf}"
