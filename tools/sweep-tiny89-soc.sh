#!/usr/bin/env bash
# SPDX-License-Identifier: MIT
#
# Seed sweep for the Tiny89 platform: place and route the same synthesis
# result at each seed and report the pixel-clock hold failures and the two
# clock margins, so a placement can be chosen by TinyNES's rule (no hold
# failure on the pixel clock, then the best margin on both clocks together).
#
# The synthesis netlist is reused, so each seed costs only place and route.
#
#   tools/sweep-tiny89-soc.sh [netlist.json] [seed ...]
#
# Eight seeds (1 to 8) unless a list is given. tools/build-tiny89-soc.sh runs
# this itself and packs the first seed whose routing passes its gates.
#
set -uo pipefail

root=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
json="${1:-$root/build/tiny89_soc_gray/tiny89_soc.json}"
shift || true
seeds=("$@")
if [[ ${#seeds[@]} -eq 0 ]]; then
    seeds=($(seq 1 8))
fi
src="$root/fpga/tiny89_soc"
suite="${OSS_CAD_SUITE:-$HOME/oss-cad-suite}"
nextpnr="${NEXTPNR_HIMBAECHEL:-$HOME/tools/nextpnr-mathieufro/nextpnr-himbaechel}"
out="${TINY89_SWEEP_DIR:-$root/build/sweep}"
jobs="${SWEEP_JOBS:-2}"

[[ -f "$json" ]] || { echo "missing netlist: $json" >&2; exit 2; }
# shellcheck disable=SC1090
source "$suite/environment"
export PYTHONHOME="$suite"
export PYTHONPATH="${APICULA_FORK:-$(dirname "$root")/apicula-mathieufro}:${PYDEPS:-$HOME/tools/pydeps}:${PYTHONPATH:-}"
export NPNR_GOWIN_CLK_FIRST="${NPNR_GOWIN_CLK_FIRST:-bus_clk}"
mkdir -p "$out"

run_seed() {
    local n="$1" d="$out/seed-$n"
    mkdir -p "$d"
    "$nextpnr" \
        --json "$json" \
        --write "$d/routed.json" \
        --device GW5AST-LV138PG484AC1/I0 \
        --freq 75 \
        --vopt cst="$src/ae350_soc.cst" \
        --sdc "$src/ae350_soc.sdc" \
        --report "$d/timing.json" \
        --seed "$n" \
        >"$d/pnr.log" 2>&1
    local holds bus pix routed
    holds=$(grep -c "Hold/min time violation for clock 'posedge pix_clk'" "$d/pnr.log")
    routed=$(grep -c "Failed to route net 'pix_clk'" "$d/pnr.log")
    bus=$(python3 -c "import json;print(f\"{json.load(open('$d/timing.json'))['fmax']['bus_clk']['achieved']:.2f}\")" 2>/dev/null || echo "?")
    pix=$(python3 -c "import json
t=json.load(open('$d/timing.json'))['fmax']
print(f\"{min(v['achieved'] for v in t.values() if abs(v['constraint']-74.25)<0.05):.2f}\")" 2>/dev/null || echo "?")
    printf 'seed %-3s holds=%-3s pix_fallback=%-3s bus=%-6s pix=%-6s\n' "$n" "$holds" "$routed" "$bus" "$pix"
}
printf 'sweep: seeds %s, %s at a time\n' "${seeds[*]}" "$jobs"
declare -a pids=()
for n in "${seeds[@]}"; do
    run_seed "$n" &
    pids+=("$!")
    if (( ${#pids[@]} >= jobs )); then
        wait -n
    fi
done
wait
echo "sweep done: $out"
