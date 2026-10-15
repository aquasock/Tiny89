#!/usr/bin/env bash
# SPDX-License-Identifier: MIT
#
# Host build + run of the Tiny89 four-level grayscale LCD tests
# (firmware/common/t89_lcd.c and firmware/common/t89_lcd_gray.c).
# No board and no calculator needed; nothing is written outside build/.
set -euo pipefail

root="$(git rev-parse --show-toplevel)"
out_dir="$root/build/tests"
mkdir -p "$out_dir"

cc="${CC:-cc}"

# TiEmu's globals (ngc, lcd_planebufs[]) are defined by the test, so the module
# needs no TiEmu headers or objects: only the module itself and its own header.
"$cc" -std=gnu11 -O2 -Wall -Wextra -Werror \
    -I"$root/firmware/common" \
    "$root/tests/test_t89_lcd.c" \
    "$root/firmware/common/t89_lcd.c" \
    "$root/firmware/common/t89_lcd_gray.c" \
    -o "$out_dir/t89_lcd_test"

"$out_dir/t89_lcd_test"
