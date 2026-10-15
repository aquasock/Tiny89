#!/usr/bin/env bash
# SPDX-License-Identifier: MIT
#
# Host build + run of the Wii Classic Controller -> TI-89 keypad mapping tests
# (firmware/common/t89_pad.c).  No board needed; nothing is written outside
# build/.
set -euo pipefail

root="$(git rev-parse --show-toplevel)"
out_dir="$root/build/tests"
mkdir -p "$out_dir"

cc="${CC:-cc}"

"$cc" -std=gnu11 -O2 -Wall -Wextra -Werror \
    -I"$root/firmware/common" \
    -I"$root/third_party/tiemu/tiemu/trunk/src/core/ti_hw" \
    "$root/tests/test_t89_pad.c" \
    "$root/firmware/common/t89_pad.c" \
    -o "$out_dir/t89_pad_test"

"$out_dir/t89_pad_test"
