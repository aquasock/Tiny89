#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-2.0-or-later

set -euo pipefail

readonly TIEMU_URL="https://github.com/debrouxl/tiemu.git"
readonly TIEMU_COMMIT="2b8df9280d2054e11ffcf7e57a73899b3ce7a7e5"

repo_root="$(git rev-parse --show-toplevel)"
destination="${1:-${repo_root}/third_party/tiemu}"

if [[ -e "${destination}" && ! -d "${destination}/.git" ]]; then
    echo "error: ${destination} exists but is not a Git checkout" >&2
    exit 1
fi

if [[ ! -d "${destination}/.git" ]]; then
    mkdir -p "$(dirname "${destination}")"
    git clone --filter=blob:none --no-checkout "${TIEMU_URL}" "${destination}"
fi

git -C "${destination}" fetch --depth=1 origin "${TIEMU_COMMIT}"
git -C "${destination}" checkout --detach "${TIEMU_COMMIT}"

actual="$(git -C "${destination}" rev-parse HEAD)"
if [[ "${actual}" != "${TIEMU_COMMIT}" ]]; then
    echo "error: expected TiEmu ${TIEMU_COMMIT}, got ${actual}" >&2
    exit 1
fi

echo "TiEmu source ready: ${destination} (${actual})"
