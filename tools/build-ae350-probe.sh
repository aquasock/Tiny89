#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-2.0-or-later

set -euo pipefail

repo_root="$(git rev-parse --show-toplevel)"
tiemu_root="${TIEMU_ROOT:-${repo_root}/third_party/tiemu}"
uae_root="${tiemu_root}/tiemu/trunk/src/core/uae"

"${repo_root}/tools/fetch-tiemu.sh" "${tiemu_root}"
make -C "${uae_root}" gen

# The BIOS is not built into this program any more: the emulator asks the
# loader for the .89u off the card and converts it itself
# (firmware/common/t89_bios.c), so nothing here needs the private file.
make -C "${repo_root}/firmware/apps/tiemu_smoke" TIEMU_ROOT="${tiemu_root}"

elf="${repo_root}/build/firmware/apps/tiemu_smoke/tiemu_smoke.elf"
echo "AE350 diagnostic ready: ${elf}"
echo "put it on the card, then: ae350run <app on card> /root/media/bios/TI89Titanium_OS.89u"
