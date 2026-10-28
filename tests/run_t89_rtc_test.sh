#!/usr/bin/env bash
# SPDX-License-Identifier: MIT
#
# The board's clock as the calculator sees it: plain C, no board and no RISC-V
# toolchain.  T89_RTC_HOST is what swaps the platform's registers and cycle
# counter for the test's stand-ins; poll() and now() stay the board's own code.
set -euo pipefail

root="$(git rev-parse --show-toplevel)"
out="$root/build/tests"
mkdir -p "$out"

"${CC:-cc}" -std=gnu11 -O2 -Wall -Wextra -Werror -DT89_RTC_HOST \
    -I"$root/firmware/common" \
    "$root/tests/test_t89_rtc.c" "$root/firmware/common/t89_rtc.c" \
    -o "$out/t89_rtc_test"

exec "$out/t89_rtc_test"
