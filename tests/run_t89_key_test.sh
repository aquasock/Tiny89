#!/usr/bin/env bash
# SPDX-License-Identifier: MIT
#
# The keyboard mapping's host test: plain C, no board and no RISC-V toolchain.
set -euo pipefail

root="$(git rev-parse --show-toplevel)"
out="$root/build/tests"
mkdir -p "$out"

"${CC:-cc}" -std=gnu11 -O2 -Wall -Wextra -Werror \
    -I"$root/firmware/common" \
    -I"$root/third_party/tiemu/tiemu/trunk/src/core/ti_hw" \
    "$root/tests/test_t89_key.c" "$root/firmware/common/t89_key.c" \
    -o "$out/t89_key_test"

exec "$out/t89_key_test"
