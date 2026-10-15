#!/usr/bin/env bash
# SPDX-License-Identifier: MIT
#
# Build Tiny89's own AE350 platform image from fpga/tiny89_soc/ with the open
# flow: Yosys, a GW5AST-138C-capable nextpnr-himbaechel and Apicula's packer.
# No Gowin EDA and no generated vendor IP.
#
# The fabric is forked from TinyNES (see THIRD_PARTY.md), so TinyNES's measured
# rules for this chip are kept here in substance and no image is packed unless
# every one of them holds:
#
#   * the A25's bus clock must be routed first, so it lands on global branch
#     GB10; the bus dies when it reaches the AE350 through GB20, and the cause
#     is not known;
#   * the AE350's dedicated PLL hop, AE350_CORE_CLK_PLLR0, must be present;
#   * the 75 MHz bus and the 74.25 MHz HDMI pixel clock must both close;
#   * every block RAM in the video unit must be written and read on one clock
#     (two-clock block RAM reads back wrong in the open flow on this chip) and
#     must use a 9-bit mode (only those were shown to read back correctly);
#   * the pixel clock may reach the clock spines only through the one bridge
#     path measured to work, SPINE26 <= CBRIDGEOUT_BOTTOM2.
#
# Placement decides whether the pixel clock can be routed inside those rules:
# on this chip some placements give it hold violations, and TinyNES's rule for
# choosing is reproduced here. By default the build therefore places and routes
# EIGHT seeds (1 to 8) over one synthesis result -- each is visible below as it
# runs, and the seeds that place cleanly are then tried in order of how well
# the pixel clock is routed until one passes every gate above. That seed's
# routing is what gets packed. Set TINY89_SOC_SWEEP=0 to build a single seed.
#
#   tools/build-tiny89-soc.sh                    synthesise, sweep 8 seeds, pack
#   TINY89_SEED=N tools/build-tiny89-soc.sh      one seed, no sweep, pack it
#   TINY89_SOC_SWEEP=0 tools/build-tiny89-soc.sh same, seed 11
#   TINY89_SOC_DIAG=1|2 tools/build-tiny89-soc.sh
#       1 holds the A25 in reset, 2 lets it run but drops its UART bytes; the
#       fabric's NOBOOT report is then the only output, for bring-up.
set -euo pipefail

root=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
src="$root/fpga/tiny89_soc"
case "${TINY89_SOC_DIAG:-0}" in
    0) synth_param=""; default_out="$root/build/tiny89_soc" ;;
    1) synth_param="chparam -set CPU_HOLD 1 ae350_soc_top"; default_out="$root/build/tiny89_soc_hold" ;;
    2) synth_param="chparam -set CPU_TX_DROP 1 ae350_soc_top"; default_out="$root/build/tiny89_soc_drop" ;;
    *) echo "TINY89_SOC_DIAG must be 0, 1 or 2" >&2; exit 2 ;;
esac
out="${TINY89_BUILD_DIR:-$default_out}"
suite="${OSS_CAD_SUITE:-$HOME/oss-cad-suite}"
nextpnr="${NEXTPNR_HIMBAECHEL:-$HOME/tools/nextpnr-mathieufro/nextpnr-himbaechel}"
apycula="${APICULA_FORK:-$(dirname "$root")/apicula-mathieufro}"
pydeps="${PYDEPS:-$HOME/tools/pydeps}"
device=GW5AST-LV138PG484AC1/I0
family=GW5AST-138C
sweep_seeds="${TINY89_SWEEP_SEEDS:-1 2 3 4 5 6 7 8}"
single_seed="${TINY89_SEED:-11}"
if [[ -n "${TINY89_SEED:-}" || "${TINY89_SOC_SWEEP:-1}" == "0" ]]; then
    do_sweep=0
else
    do_sweep=1
fi

[[ -f "$suite/environment" ]] || { echo "missing open toolchain: $suite" >&2; exit 2; }
# shellcheck disable=SC1090
source "$suite/environment"
export PYTHONHOME="$suite"
export PYTHONPATH="$apycula:$pydeps:${PYTHONPATH:-}"
export NPNR_GOWIN_CLK_FIRST="${NPNR_GOWIN_CLK_FIRST:-bus_clk}"

[[ -x "$nextpnr" ]] || { echo "missing open nextpnr: $nextpnr" >&2; exit 2; }
[[ -f "$apycula/apycula/$family.msgpack.xz" ]] || {
    echo "missing local Apicula device database in $apycula/apycula" >&2; exit 2;
}
mkdir -p "$out"

echo "Boot ROM firmware"
make -s -C "$root/firmware/boot"

# The boot ROM's INIT_FILE and the desk layer's FONT_FILE are relative to the
# repository root.
cd "$root"
echo "Yosys synthesis"
yosys -Q -T -q -l "$out/synth.log" -p "
    read_verilog -lib $suite/share/yosys/gowin/cells_xtra_gw5a.v
    read_verilog -sv $src/ahb_boot_rom.sv $src/ahb_ram.sv $src/ahb_sdram.sv $src/exts_uart.sv $src/desk_link.sv $src/video.sv $src/wii_pads.sv $src/i2s_out.sv $src/ae350_soc.sv
    $synth_param
    synth_gowin -family gw5a -top ae350_soc_top
    write_json $out/tiny89_soc.json
"
netlist="$out/tiny89_soc.json"

# Every gate above, on one seed's routing. 0 = packable.
run_gates() {
    local routed="$1" timing="$2"
    "$suite/py3bin/python3" - "$routed" "$timing" <<'PY'
import json
import sys

routed, timing_path = sys.argv[1], sys.argv[2]
design = json.load(open(routed))
route = design["modules"]["top"]["netnames"]["core_clk"]["attributes"]["ROUTING"]
if "AE350_CORE_CLK_PLLR0" not in route:
    raise SystemExit("AE350 dedicated PLL clock hop is absent from routed design")
timing = json.load(open(timing_path))["fmax"]
bus = timing["bus_clk"]
if bus["constraint"] < 75 or bus["achieved"] < 75:
    raise SystemExit(f"75 MHz AHB bus timing failed: {bus}")
pixel = [t for t in timing.values() if abs(t["constraint"] - 74.25) < 0.05]
if not pixel:
    raise SystemExit("HDMI pixel clock was not constrained at 74.25 MHz")
if min(t["achieved"] for t in pixel) < 74.25:
    raise SystemExit(f"74.25 MHz HDMI pixel clock timing failed: {pixel}")
cells = design["modules"]["top"]["cells"]
video_rams = {n: c for n, c in cells.items() if n.startswith("screen.")
              and any(p.startswith("CLKB") for p in c["connections"])}
two_clock = sorted(n for n, c in video_rams.items()
                   if c["connections"]["CLKA"] != c["connections"]["CLKB"])
if two_clock:
    raise SystemExit(f"video block RAMs on two clocks: {two_clock[:4]}")
other_modes = sorted(n for n, c in cells.items() if n.startswith("screen.")
                     and any(k.startswith("BIT_WIDTH") and int(v, 2) % 9
                             for k, v in c["parameters"].items()))
if other_modes:
    raise SystemExit(f"video block RAMs not in 9-bit mode: {other_modes[:4]}")
pix_route = design["modules"]["top"]["netnames"]["pix_clk"]["attributes"]["ROUTING"].split(";")
bridges = sorted({p.split("/", 1)[1] for p in pix_route[1::3] if "/CBRIDGEOUT" in p})
if bridges != ["SPINE26/CBRIDGEOUT_BOTTOM2"]:
    raise SystemExit(f"pixel clock uses unmeasured clock bridge pips: {bridges}")
print(f"  gates: AE350 PLL hop present; bus {bus['achieved']:.2f} MHz; "
      f"pixel {min(t['achieved'] for t in pixel):.2f} MHz; "
      f"{len(video_rams)} video block RAMs, one clock, 9-bit; "
      f"pixel clock bridge SPINE26 <= CBRIDGEOUT_BOTTOM2 only")
PY
}

place_and_route() {
    local seed="$1" dir="$2"
    mkdir -p "$dir"
    echo "  seed $seed: placing and routing"
    "$nextpnr" \
        --json "$netlist" \
        --write "$dir/tiny89_soc-routed.json" \
        --device "$device" \
        --freq 75 \
        --vopt cst="$src/ae350_soc.cst" \
        --sdc="$src/ae350_soc.sdc" \
        --report "$dir/timing.json" \
        --seed "$seed" \
        >"$dir/nextpnr.log" 2>&1 || true
    grep -q "routing '$NPNR_GOWIN_CLK_FIRST' first" "$dir/nextpnr.log" || {
        echo "  seed $seed: nextpnr did not route $NPNR_GOWIN_CLK_FIRST first" >&2
    }
}

# Seeds that placed and routed, ordered by TinyNES's rule: no pixel-clock hold
# failure first, then the fewest pixel-clock pins off dedicated routing, then
# the best margin on the two clocks together.
rank_seeds() {
    "$suite/py3bin/python3" - "$out" <<'PY'
import json
import pathlib
import subprocess
import sys

out = pathlib.Path(sys.argv[1])
ranked = []
for d in sorted((out / "sweep").glob("seed-*"), key=lambda p: int(p.name.split("-")[1])):
    log = (d / "pnr.log").read_text() if (d / "pnr.log").exists() else ""
    holds = log.count("Hold/min time violation for clock 'posedge pix_clk'")
    fallbacks = log.count("Failed to route net 'pix_clk'")
    routed = d / "routed.json"
    timing = d / "timing.json"
    if not routed.exists() or not timing.exists():
        continue
    t = json.loads(timing.read_text()).get("fmax", {})
    try:
        bus = t["bus_clk"]["achieved"]
        pixel = min(v["achieved"] for v in t.values()
                    if abs(v["constraint"] - 74.25) < 0.05)
    except (KeyError, ValueError):
        continue
    ranked.append((holds, fallbacks, -min(bus, pixel), int(d.name.split("-")[1]),
                   bus, pixel))
ranked.sort()
for holds, fallbacks, _, seed, bus, pixel in ranked:
    print(f"{seed} {holds} {fallbacks} {bus:.2f} {pixel:.2f}")
PY
}

winner=""
if (( do_sweep )); then
    netlist_sha=$(sha256sum "$netlist" | cut -d' ' -f1)
    sweep_ready=1
    for seed in $sweep_seeds; do
        [[ -f "$out/sweep/seed-$seed/routed.json" && -f "$out/sweep/seed-$seed/timing.json" ]] || sweep_ready=0
    done
    if (( sweep_ready )) && [[ -f "$out/sweep/netlist.sha256" ]] &&
       [[ "$(cat "$out/sweep/netlist.sha256")" == "$netlist_sha" ]]; then
        echo "Seed sweep: reusing $out/sweep, the synthesis result is unchanged"
    else
        echo "Seed sweep: $sweep_seeds (each is placed and routed on the one synthesis result)"
        # shellcheck disable=SC2086
        TINY89_SWEEP_DIR="$out/sweep" SWEEP_JOBS="${TINY89_SWEEP_JOBS:-4}" \
            "$root/tools/sweep-tiny89-soc.sh" "$netlist" $sweep_seeds || true
        printf '%s\n' "$netlist_sha" > "$out/sweep/netlist.sha256"
    fi
    echo "Sweep results (seed holds pixel-clock-fallbacks bus_MHz pixel_MHz):"
    rank_seeds | while read -r seed holds fallbacks bus pixel; do
        printf '  %-4s holds=%-3s fallbacks=%-4s bus=%-6s pixel=%-6s\n' \
            "$seed" "$holds" "$fallbacks" "$bus" "$pixel"
    done
    while read -r seed holds fallbacks bus pixel; do
        echo "Trying seed $seed (holds=$holds fallbacks=$fallbacks)"
        if run_gates "$out/sweep/seed-$seed/routed.json" \
                     "$out/sweep/seed-$seed/timing.json"; then
            winner="$seed"
            cp "$out/sweep/seed-$seed/routed.json" "$out/tiny89_soc-routed.json"
            cp "$out/sweep/seed-$seed/timing.json" "$out/timing.json"
            cp "$out/sweep/seed-$seed/pnr.log" "$out/nextpnr.log"
            break
        fi
        echo "  seed $seed did not pass the gates" >&2
    done < <(rank_seeds)
    if [[ -z "$winner" ]]; then
        echo "No swept seed passes every gate; no image packed." >&2
        echo "Run tools/sweep-tiny89-soc.sh with more seeds, or change the design." >&2
        exit 1
    fi
    echo "Chosen placement: seed $winner"
else
    echo "Single placement: seed $single_seed (no sweep)"
    place_and_route "$single_seed" "$out"
    run_gates "$out/tiny89_soc-routed.json" "$out/timing.json" || exit 1
    winner="$single_seed"
fi

echo "Apicula bitstream packing"
"$suite/py3bin/python3" -m apycula.gowin_pack \
    -d "$family" \
    -o "$out/tiny89_soc.fs" \
    "$out/tiny89_soc-routed.json" \
    >"$out/pack.log" 2>&1 || {
        tail -80 "$out/pack.log" >&2
        exit 1
    }

# One-wire SRAM loads take the text .fs; TinyTang's tangload in two-wire mode
# takes the same bits packed eight to a byte as a .bin on the SD card.
"$suite/py3bin/python3" "$root/tools/fs_to_bin.py" "$out/tiny89_soc.fs" -o "$out/tiny89_soc.bin" >/dev/null

echo "Packed seed $winner"
sha256sum "$out/tiny89_soc.fs" "$out/tiny89_soc.bin"
echo "Ready to SRAM load (one-wire): $out/tiny89_soc.fs"
echo "Ready to tangload (two-wire):  $out/tiny89_soc.bin"
