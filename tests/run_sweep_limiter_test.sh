#!/usr/bin/env bash
# SPDX-License-Identifier: MIT
#
# Test the seed sweep's concurrency limiter.
#
# The sweep places and routes one seed per placement, and SWEEP_JOBS says how
# many may be in flight at once.  This test runs the real script against a stub
# placer and checks the two things the limiter is for: that every seed is
# placed, and that no more than SWEEP_JOBS placements overlap in time.
#
# It exists because the limiter was once wrong in a way that produced no
# placement at all.  "${pids[@]:-}" on an empty array expands to one EMPTY
# element rather than to nothing, so the loop's own count was 1 with no child
# running, `wait -n` was called on an empty set, and the script span at 100% of
# a core without ever starting nextpnr.  A test that only looked for the
# script's final line, or that trusted its exit status, would have missed it:
# the run has to be shown to place seeds, and to bound how many at once.
#
#   tests/run_sweep_limiter_test.sh
set -uo pipefail

root="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
work="$(mktemp -d)"
trap 'rm -rf "$work"' EXIT

fails=0
checks=0
check() {
    local ok="$1" what="$2"
    checks=$((checks + 1))
    if [[ "$ok" == "1" ]]; then
        printf '  ok   %s\n' "$what"
    else
        printf '  FAIL %s\n' "$what"
        fails=$((fails + 1))
    fi
}

# The sweep sources "$OSS_CAD_SUITE/environment", so the stub suite needs one.
mkdir -p "$work/suite"
: > "$work/suite/environment"

# A stub placer: it records the window it was alive for, sleeps, and writes the
# two files the sweep reads back (a timing report and a placement log).  It is
# deliberately slower than the limiter's own bookkeeping, so a limiter that let
# everything go at once would overlap by seconds.
cat > "$work/nextpnr-stub" <<'STUB'
#!/usr/bin/env bash
write=""; report=""
while (($#)); do
    case "$1" in
        --write)  write="$2";  shift 2 ;;
        --report) report="$2"; shift 2 ;;
        *) shift ;;
    esac
done
mkdir -p "$(dirname "$write")"
printf 'START %s %s\n' "$$" "$(date +%s%N)" >> "$SWEEP_TEST_LEDGER"
sleep 1
: > "$write"
printf '{"fmax":{"bus_clk":{"constraint":75,"achieved":90.00},' > "$report"
printf '"pixel":{"constraint":74.25,"achieved":80.00}}}\n' >> "$report"
: > "${write%/*}/pnr.log"
printf 'END %s %s\n' "$$" "$(date +%s%N)" >> "$SWEEP_TEST_LEDGER"
STUB
chmod +x "$work/nextpnr-stub"

: > "$work/netlist.json"

# How many placements overlapped, from the stub's own ledger.
max_concurrency() {
    python3 - "$1" <<'PY'
import sys
starts, ends, events = {}, {}, []
for line in open(sys.argv[1]):
    kind, pid, stamp = line.split()
    stamp = int(stamp)
    (starts if kind == "START" else ends)[pid] = stamp
    events.append((stamp, 1 if kind == "START" else -1))
live = best = 0
for _, delta in sorted(events):
    live += delta
    best = max(best, live)
print(best)
PY
}

run_sweep() {
    local jobs="$1" dir="$2" ledger="$3" seconds="$4"
    : > "$ledger"
    TINY89_SWEEP_DIR="$dir" \
    NEXTPNR_HIMBAECHEL="$work/nextpnr-stub" \
    OSS_CAD_SUITE="$work/suite" \
    SWEEP_TEST_LEDGER="$ledger" \
    SWEEP_JOBS="$jobs" \
    timeout "$seconds" "$root/tools/sweep-tiny89-soc.sh" "$work/netlist.json" 1 2 3 4 \
        >"$dir.log" 2>&1
    return $?
}

echo "sweep limiter:"
for jobs in 1 2; do
    dir="$work/sweep-$jobs"
    ledger="$work/ledger-$jobs"
    # Four seeds at a second each; the timeout is generous for three placements
    # in series and far too short for a run that never places anything.
    if run_sweep "$jobs" "$dir" "$ledger" 60; then
        check 1 "SWEEP_JOBS=$jobs: the sweep finished"
    else
        check 0 "SWEEP_JOBS=$jobs: the sweep finished (exit $?; a spin here is the empty-array bug)"
    fi

    placed=0
    for n in 1 2 3 4; do
        [[ -f "$dir/seed-$n/routed.json" && -f "$dir/seed-$n/timing.json" ]] && placed=$((placed + 1))
    done
    check "$([[ $placed -eq 4 ]] && echo 1 || echo 0)" "SWEEP_JOBS=$jobs: all four seeds placed (got $placed)"

    seen="$(max_concurrency "$ledger")"
    if [[ "$jobs" == "1" ]]; then
        check "$([[ ${seen:-0} -eq 1 ]] && echo 1 || echo 0)" "SWEEP_JOBS=1: exactly one placement at a time (saw $seen)"
    else
        # Two at a time is allowed, and expected here: four one-second
        # placements under a two-slot limit overlap.
        check "$([[ ${seen:-0} -ge 1 && ${seen:-0} -le 2 ]] && echo 1 || echo 0)" "SWEEP_JOBS=2: never more than two at once (saw $seen)"
    fi
done

printf 'sweep limiter: %s (%d checks)\n' "$([[ $fails -eq 0 ]] && echo PASS || echo FAIL)" "$checks"
exit $([[ $fails -eq 0 ]] && echo 0 || echo 1)
