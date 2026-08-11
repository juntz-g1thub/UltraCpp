#!/usr/bin/env bash
# bench.sh — M0 compile-time performance baseline
#
# Spec: .dev/plans/0.4.0-test-milestones.md §4.3 (template) + §5
#
# For every *.uc in test/programs/baseline/ (except m0_44_helper.uc),
# runs `uc_lexer --build` N times (default 3, override with BENCH_PASSES=N)
# and records wall-clock duration per pass + integer average.
#
# Output: CSV to stdout. Redirect to test/bench/m0.csv when invoking:
#   bash test/bench/bench.sh > test/bench/m0.csv
#
# We measure the FULL end-to-end build (`uc_lexer --build`), not just
# lex/parse — that is what the §4.3 template measures, and the spec's
# performance regression threshold (§5, 20% delta) is against compile_ms.
#
# PARSE-ONLY files (m0_34 / m0_35 / m0_36) are NOT skipped here — we only
# care about compile time, not runtime semantics.
#
# Exit codes:
#   0  all measurements captured (compile failures are recorded, not fatal)
#   2  preconditions not met (compiler missing, test dir missing, ...)

set -euo pipefail

# ─── Configuration ───────────────────────────────────────────────────────
UC="src-c/build/uc_lexer"
TEST_DIR="${1:-test/programs/baseline}"
PASSES="${BENCH_PASSES:-3}"

EXCLUDED_FILES=(
    "m0_44_helper.uc"   # paired with m0_44_multiple_files.uc
)

# ─── Helpers ─────────────────────────────────────────────────────────────
is_excluded() {
    local needle="$1"
    local item
    for item in "${EXCLUDED_FILES[@]}"; do
        if [[ "$item" == "$needle" ]]; then
            return 0
        fi
    done
    return 1
}

# ─── Cleanup trap ────────────────────────────────────────────────────────
CURRENT_OUT_DIR=""
cleanup() {
    if [[ -n "$CURRENT_OUT_DIR" ]] && [[ -d "$CURRENT_OUT_DIR" ]]; then
        rm -rf "$CURRENT_OUT_DIR"
    fi
}
trap cleanup EXIT

# ─── Preconditions ───────────────────────────────────────────────────────
if [[ ! -x "$UC" ]]; then
    echo "ERROR: compiler not built: $UC" >&2
    echo "  Fix: make -C src-c" >&2
    exit 2
fi
if [[ ! -d "$TEST_DIR" ]]; then
    echo "ERROR: test directory not found: $TEST_DIR" >&2
    echo "  Hint: run from the repository root." >&2
    exit 2
fi

# ─── CSV header ──────────────────────────────────────────────────────────
header="file"
for i in $(seq 1 "$PASSES"); do
    header+=",duration_ms_pass${i}"
done
header+=",avg_ms"
echo "$header"

# ─── Main loop ───────────────────────────────────────────────────────────
for src in "$TEST_DIR"/*.uc; do
    [[ -e "$src" ]] || continue
    name=$(basename "$src")

    if is_excluded "$name"; then
        continue
    fi

    pass_durations=()
    any_compile_failed=0

    for i in $(seq 1 "$PASSES"); do
        CURRENT_OUT_DIR=$(mktemp -d)

        start_ns=$(date +%s%N)
        # Relax `set -e` for this call: a failing compile must not abort
        # the whole bench loop. We still record the elapsed time so the
        # orchestrator can see the spike (or zero-cost fast-fail).
        if ! "$UC" --build "$src" -o "$CURRENT_OUT_DIR/exe" \
                >/dev/null 2>&1; then
            any_compile_failed=1
        fi
        end_ns=$(date +%s%N)

        pass_durations+=( $(( (end_ns - start_ns) / 1000000 )) )

        cleanup
        CURRENT_OUT_DIR=""
    done

    # Integer average (truncated).
    sum=0
    for d in "${pass_durations[@]}"; do
        sum=$((sum + d))
    done
    avg=$(( sum / PASSES ))

    # Emit CSV row.
    row="$name"
    for d in "${pass_durations[@]}"; do
        row+=",$d"
    done
    row+=",$avg"
    # Compile failures are reported via run_baseline.sh; here we just
    # tag the row so the human reader can spot it without a separate run.
    if [[ $any_compile_failed -eq 1 ]]; then
        row+="  # compile_failed"
    fi
    echo "$row"
done
