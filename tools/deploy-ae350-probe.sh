#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-2.0-or-later

set -euo pipefail

repo_root="$(git rev-parse --show-toplevel)"
tinynes="${TINYNES_DIR:-$(dirname "${repo_root}")/TinyNES}"
port="${TINYTANG_PORT:-/dev/ttyACM0}"
elf="${1:-${repo_root}/build/firmware/apps/tiemu_smoke/tiemu_smoke.elf}"

if [[ ! -f "${elf}" ]]; then
    echo "error: diagnostic ELF not found: ${elf}" >&2
    exit 1
fi
if [[ ! -f "${tinynes}/tools/load_ae350.py" ]]; then
    echo "error: TinyNES loader not found; set TINYNES_DIR" >&2
    exit 1
fi

python3 -u "${tinynes}/tools/load_ae350.py" \
    --tinytang --port "${port}" --timeout 10 --run-timeout 180 \
    --expect "Tiny89 READY" "${elf}"
