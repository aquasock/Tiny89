#!/usr/bin/env bash
# SPDX-License-Identifier: MIT
#
# The BIOS conversion, against the real upgrade file.
#
# The C in firmware/common/t89_bios.c has to agree byte for byte with
# tools/convert_89u.py, because that tool's output is what earlier cycles
# booted and what the emulator was judged on.  The upgrade file is proprietary
# and lives outside this repository, so when there is none to read this test
# says so and still runs the conversion's own refusals.
#
#   tests/run_t89_bios_test.sh [path-to-.89u]
#   T89_OS_IMAGE=/path/to/TI89Titanium_OS.89u tests/run_t89_bios_test.sh
set -uo pipefail

root="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
work="$(mktemp -d)"
trap 'rm -rf "$work"' EXIT

os_image="${1:-${T89_OS_IMAGE:-}}"
if [[ -z "$os_image" ]]; then
    for candidate in "$HOME/Desktop/TI89Titanium_OS.89u" \
                     "$(dirname "$root")/TI89Titanium_OS.89u"; do
        if [[ -f "$candidate" ]]; then
            os_image="$candidate"
            break
        fi
    done
fi

if ! command -v cc >/dev/null 2>&1; then
    echo "run_t89_bios_test: no C compiler" >&2
    exit 1
fi

cc -std=c11 -O1 -Wall -Wextra -Werror -I "$root/firmware/common" \
   -o "$work/tb" "$root/tests/test_t89_bios.c" \
   "$root/firmware/common/t89_bios.c" || exit 1

"$work/tb" --checks || exit 1

if [[ -z "$os_image" || ! -f "$os_image" ]]; then
    echo "t89_bios: PASS (refusals only; no private upgrade file to compare against)"
    echo "          pass the path or set T89_OS_IMAGE to check the conversion too"
    exit 0
fi

"$work/tb" "$os_image" "$work/c.raw" || exit 1
python3 "$root/tools/convert_89u.py" "$os_image" "$work/py.raw" >/dev/null || exit 1

if cmp -s "$work/c.raw" "$work/py.raw"; then
    printf 't89_bios: PASS (%s bytes of upgrade -> %s bytes of flash; the C and tools/convert_89u.py agree byte for byte)\n' \
        "$(stat -c%s "$os_image")" "$(stat -c%s "$work/c.raw")"
else
    echo "t89_bios: FAIL (the C conversion and tools/convert_89u.py differ)"
    cmp "$work/c.raw" "$work/py.raw" | head -3
    exit 1
fi
