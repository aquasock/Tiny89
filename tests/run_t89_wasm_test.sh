#!/usr/bin/env bash
# SPDX-License-Identifier: MIT
#
# The browser build's checks, on the host: does it boot the operating system to
# the same desktop the board reaches, and does a file install through the
# emulated link?
#
#   tests/run_t89_wasm_test.sh                    boot check
#   tests/run_t89_wasm_test.sh --install f.89y    install one or more files
#
# Needs node and a build (tools/build-wasm-app.sh).  The TI-89 upgrade file is
# private and is looked for at $TI89_OS, then ~/Desktop and ~/Downloads; with
# none found the boot check says so and passes, like the other tests in here
# that need the user's own file.
set -euo pipefail

root="$(git rev-parse --show-toplevel)"

if ! command -v node >/dev/null 2>&1; then
    echo "wasm test: node not found" >&2
    exit 1
fi
if [[ ! -f "$root/web/dist/t89.js" ]]; then
    echo "wasm test: no build; run tools/build-wasm-app.sh" >&2
    exit 1
fi

if [[ "${1:-}" == "--state" ]]; then
    shift
    exec node "$root/tests/t89_wasm_state_test.js" --state "$@"
fi

if [[ "${1:-}" == "--install" ]]; then
    shift
    exec node "$root/tests/t89_wasm_install_test.js" --install "$@"
fi

exec node "$root/tests/t89_wasm_boot_test.js"
