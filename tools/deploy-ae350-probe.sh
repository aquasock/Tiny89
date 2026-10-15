#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-2.0-or-later
#
# Load the diagnostic into the AE350's SDRAM over TinyTang's console pipe and
# let it run for a bounded window.
#
# The program is continuous: it presents frames until the board is reset and
# never exits, so TinyNES's loader can never print its own PASS (that needs a
# `TinyNES EXIT 0` line) and its "program still running after N s" failure is
# the expected outcome once the marker has been seen.  What this script
# requires instead is the marker, and the absence of a line starting with BAD.
# It also needs the AE350 core image to be the running FPGA core: load it with
# `tangload /root/cores/console138k/tiny89-ae350.bin` on TinyTang's console
# first, since the AE350's boot ROM must be the thing on the other end of the
# pipe.  A power cycle reloads whatever core the FPGA's own flash holds, so
# the core and then this program are loaded again after one.
#
#   tools/deploy-ae350-probe.sh [elf]           RUN_SECONDS=300 by default
#
set -euo pipefail

repo_root="$(git rev-parse --show-toplevel)"
tinynes="${TINYNES_DIR:-$(dirname "${repo_root}")/TinyNES}"
port="${TINYTANG_PORT:-/dev/ttyACM0}"
elf="${1:-${repo_root}/build/firmware/apps/tiemu_smoke/tiemu_smoke.elf}"
run_seconds="${RUN_SECONDS:-300}"
marker="${MARKER:-Tiny89 READY}"

if [[ ! -f "${elf}" ]]; then
    echo "error: diagnostic ELF not found: ${elf}" >&2
    exit 1
fi
if [[ ! -f "${tinynes}/tools/load_ae350.py" ]]; then
    echo "error: TinyNES loader not found; set TINYNES_DIR" >&2
    exit 1
fi

echo "deploying $(sha256sum "${elf}" | cut -d' ' -f1) as $(basename "${elf}")"

log="$(mktemp -t tiny89-deploy.XXXXXX)"
trap 'rm -f "${log}"' EXIT

set +e
python3 -u "${tinynes}/tools/load_ae350.py" \
    --tinytang --port "${port}" --timeout 15 --run-timeout "${run_seconds}" \
    --expect "${marker}" "${elf}" >"${log}" 2>&1
loader_status=$?
set -e
cat "${log}"

if grep -qE '^(BAD|NOBOOT )' "${log}"; then
    echo "error: the program or the boot ROM reported a fault" >&2
    exit 1
fi
if ! grep -qF "${marker}" "${log}"; then
    echo "error: ${marker} never appeared in ${run_seconds}s (loader exit ${loader_status})" >&2
    exit 1
fi
if grep -qF "FAIL: no 'TinyNES LOAD' line" "${log}"; then
    echo "error: the AE350's boot ROM is not on the other end of the pipe;" \
         "load the core image with tangload first" >&2
    exit 1
fi

echo "AE350 diagnostic running: $(basename "${elf}") reached ${marker}"
if [[ ${loader_status} -ne 0 ]]; then
    echo "(the loader's own exit ${loader_status} is its 'still running' result," \
         "which a continuous program always gives)"
fi
