#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-2.0-or-later
#
# Apply Tiny89's one change to the fetched TiEmu tree.
#
# The tree is fetched clean by tools/fetch-tiemu.sh and is not in version
# control here, so anything this project needs to change in it has to be
# applied by a script.  There is exactly one such change today.
#
#   hw.c defines `void hw_update(void)` while hw.h declares
#   `int hw_update(void)`, and every caller discards the value. Nothing reads
#   a return register that was never set on x86 or RV32, so it went unnoticed
#   there; emscripten does not tolerate it -- wasm-ld reports "function
#   signature mismatch: hw_update" and the call becomes a trap the first time
#   do_cycles reaches it, which is why the browser build would not run a single
#   chunk until the definition followed its own header.
#
# Idempotent: applying it twice changes nothing.  Called by fetch-tiemu.sh and
# by build-wasm-app.sh, so a fresh clone builds without a manual step.
set -euo pipefail

root="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
tiemu="${TIEMU_ROOT:-$root/third_party/tiemu}"
hw="$tiemu/tiemu/trunk/src/core/ti_hw/hw.c"

if [[ ! -f "$hw" ]]; then
    echo "patch-tiemu: no TiEmu tree at $tiemu" >&2
    exit 2
fi

if grep -q '^int hw_update(void)$' "$hw"; then
    echo "patch-tiemu: already applied"
    exit 0
fi

python3 - "$hw" <<'PY'
import sys

path = sys.argv[1]
# TiEmu's sources are not all UTF-8 (a name in hw.c carries a Latin-1 byte), so
# the file is round-tripped in the encoding it is actually in.
text = open(path, encoding="latin-1").read()

signature = "void hw_update(void)\n{"
if text.count(signature) != 1:
    raise SystemExit(f"patch-tiemu: expected one definition of hw_update, found {text.count(signature)}")
text = text.replace(signature, "int hw_update(void)\n{")

# The function ends with the LCD block's closing brace; its return goes last.
tail = "\t\tlcd_hook_hw1();\n\t}\n}\n"
if text.count(tail) != 1:
    raise SystemExit(f"patch-tiemu: could not find hw_update's ending ({text.count(tail)})")
text = text.replace(tail, "\t\tlcd_hook_hw1();\n\t}\n\treturn 0;\n}\n")

open(path, "w", encoding="latin-1").write(text)
print("patch-tiemu: hw_update now returns int, as hw.h declares")
PY
