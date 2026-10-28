#!/usr/bin/env bash
# SPDX-License-Identifier: MIT
#
# The container parser's host test: plain C, no board, no private files.
set -euo pipefail

root="$(git rev-parse --show-toplevel)"
out="$root/build/tests"
mkdir -p "$out"

"${CC:-cc}" -std=gnu11 -O2 -Wall -Wextra -Werror \
    -I"$root/firmware/include" \
    "$root/tests/test_t89_file.c" "$root/firmware/common/t89_file.c" \
    -o "$out/t89_file_test"

exec "$out/t89_file_test"
