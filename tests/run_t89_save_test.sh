#!/usr/bin/env bash
# SPDX-License-Identifier: MIT
#
# The RAM snapshot's host test: plain C, no board and no RISC-V toolchain.
set -euo pipefail

root="$(git rev-parse --show-toplevel)"
out="$root/build/tests"
mkdir -p "$out"

"${CC:-cc}" -std=gnu11 -O2 -Wall -Wextra -Werror \
    -I"$root/firmware/common" \
    "$root/tests/test_t89_save.c" "$root/firmware/common/t89_save.c" \
    -o "$out/t89_save_test"

exec "$out/t89_save_test"
