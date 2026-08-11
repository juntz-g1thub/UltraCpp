#!/usr/bin/env bash
# run_baseline.sh — M0 baseline .uc test runner
#
# Spec: .dev/plans/0.4.0-test-milestones.md §4.1 (template) + §3.0 / §3.9
# Usage: bash test/e2e/run_baseline.sh
#        (must be run from the repository root)
#
# Compiles every *.uc in test/programs/baseline/ via `uc_lexer --build`,
# then runs the resulting binary (regular tests) or only checks the
# compile (PARSE-ONLY tests).
#
# Exit codes:
#   0  all PASS
#   1  one or more FAIL
#   2  preconditions not met (compiler missing, test dir missing, ...)
#
# Output:
#   stdout                          per-file ✓/✗ line + final summary
#   test/results/m0_baseline.txt    machine-readable results
#                                   (file  PASS|FAIL  duration_ms  [reason])

set -euo pipefail

# ─── Configuration ───────────────────────────────────────────────────────
UC="src-c/build/uc_lexer"
TEST_DIR="test/programs/baseline"
RESULT_FILE="test/results/m0_baseline.txt"

# PARSE-ONLY files (per spec §3.9 + §3.0 DoD): compile must succeed,
# but the resulting binary is allowed to fail or return 0 — runtime
# semantics of these keywords (alloc/free, unique, move) are verified
# in M2, not M0.
PARSE_ONLY_FILES=(
    "m0_34_alloc_free.uc"
    "m0_35_unique_keyword.uc"
    "m0_36_move_keyword.uc"
)

# Helper files paired with multi-file tests — never built standalone.
EXCLUDED_FILES=(
    "m0_44_helper.uc"
)

# ─── Helpers ─────────────────────────────────────────────────────────────
is_in_list() {
    local needle="$1"
    shift
    local item
    for item in "$@"; do
        if [[ "$item" == "$needle" ]]; then
            return 0
        fi
    done
    return 1
}

# extract_expected_exit <file.uc>
#
# Parse the .uc source and return the integer that main() is expected
# to exit with. Used to decide pass/fail at runtime (rule 1: trailing
# `// N` comment, rule 2: bare `return N;` literal, rule 4: default 0).
#
# Implementation: GNU awk (gawk 4+). We do not need to be Mac-portable
# for the CI runner — this project ships on Linux per its env spec.
extract_expected_exit() {
    local src="$1"
    local out
    # NB: we use gawk's 3-arg match() to capture the trailing-comment
    # body in one go. Brace counting uses gsub() with a regex (NOT
    # a string like `"}"`) because some gawk builds reject `gsub("}",
    # ...)` as a syntax error in a `close = gsub(...)` assignment.
    out=$(awk '
        function count_open(line)  { return gsub(/[\{]/, "", line) }
        function count_close(line) { return gsub(/[\}]/, "", line) }
        /int[[:space:]]+main[[:space:]]*\(/ {
            in_main = 1
            open  = count_open($0)
            cls   = count_close($0)
            depth += open - cls
            if (open > 0) saw_brace = 1
            next
        }
        in_main {
            open  = count_open($0)
            cls   = count_close($0)
            depth += open - cls
            if (open > 0) saw_brace = 1

            # Rule 1: trailing "// ... N" after `return EXPR;`.
            # Extract the comment, then take the last integer in it
            # (so `// 3 + 12 + 14 = 29` → 29, `// bytes = 9` → 9).
            if (match($0, /return[ \t]+[^;]*;[ \t]*\/\/[ \t]*(.*)$/, m)) {
                if (match(m[1], /(-?[0-9]+)[ \t]*$/, n)) {
                    print n[1]
                    exit
                }
            }
            # Rule 2: `return <integer-literal>;` with no comment.
            if (match($0, /return[ \t]+(-?[0-9]+)[ \t]*;/, m)) {
                print m[1]
                exit
            }
            if (saw_brace && depth == 0) exit
        }
    ' "$src")
    if [[ -z "$out" ]]; then
        echo "WARNING: could not extract expected exit code from $src, defaulting to 0" >&2
        echo 0
    else
        echo "$out"
    fi
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
    echo "  Hint: run this script from the repository root." >&2
    exit 2
fi

# ─── Init results file ───────────────────────────────────────────────────
mkdir -p "$(dirname "$RESULT_FILE")"
{
    echo "# M0 baseline results"
    echo "# Format: <file>  <PASS|FAIL>  <duration_ms>  [<reason>]"
} > "$RESULT_FILE"

# ─── Counters ────────────────────────────────────────────────────────────
TOTAL=0
PASS=0
FAIL=0
PARSE_ONLY_PASS=0
FAILED_TESTS=()

# ─── Main loop ───────────────────────────────────────────────────────────
for src in "$TEST_DIR"/*.uc; do
    [[ -e "$src" ]] || continue   # empty-glob guard
    name=$(basename "$src")

    # Skip helper files paired with multi-file tests.
    if is_in_list "$name" "${EXCLUDED_FILES[@]}"; then
        continue
    fi

    TOTAL=$((TOTAL + 1))
    parse_only=0
    if is_in_list "$name" "${PARSE_ONLY_FILES[@]}"; then
        parse_only=1
    fi

    CURRENT_OUT_DIR=$(mktemp -d)

    start_ns=$(date +%s%N)
    compile_ok=0
    run_ok=-1
    reason=""

    # ── Compile ──
    if "$UC" --build "$src" -o "$CURRENT_OUT_DIR/exe" \
            2>"$CURRENT_OUT_DIR/compile_err" >/dev/null; then
        compile_ok=1
    fi

    # ── Run (only if compile succeeded) ──
    actual_exit_code=-1
    if [[ $compile_ok -eq 1 ]]; then
        if [[ -x "$CURRENT_OUT_DIR/exe" ]]; then
            if "$CURRENT_OUT_DIR/exe" \
                    2>"$CURRENT_OUT_DIR/run_err" \
                    >"$CURRENT_OUT_DIR/run_out"; then
                run_ok=1
                actual_exit_code=0
            else
                run_rc=$?
                run_ok=0
                actual_exit_code=$run_rc
                reason="run_exit_nonzero"
            fi
        else
            reason="no_executable"
        fi
    fi

    end_ns=$(date +%s%N)
    duration_ms=$(( (end_ns - start_ns) / 1000000 ))

    # ── Decide pass / fail ──
    passed=0
    if [[ $compile_ok -eq 1 ]]; then
        if [[ $parse_only -eq 1 ]]; then
            # PARSE-ONLY: compile-only check per §3.0 DoD.
            passed=1
            PARSE_ONLY_PASS=$((PARSE_ONLY_PASS + 1))
        else
            # Runtime check: actual exit code must equal the value
            # declared in the .uc file (// comment or `return N;`
            # literal). extract_expected_exit falls back to 0 with
            # a stderr warning if nothing parseable is found.
            expected=$(extract_expected_exit "$src")
            if [[ $actual_exit_code -eq $expected ]]; then
                passed=1
            else
                reason="wrong_exit_code expected=$expected actual=$actual_exit_code"
            fi
        fi
    else
        reason="compile_failed"
    fi

    # ── Report ──
    if [[ $passed -eq 1 ]]; then
        PASS=$((PASS + 1))
        printf "✓ %-40s  %5d ms\n" "$name" "$duration_ms"
        echo "$name  PASS  $duration_ms" >> "$RESULT_FILE"
    else
        FAIL=$((FAIL + 1))
        FAILED_TESTS+=("$name")
        printf "✗ %-40s  %5d ms  (%s)\n" "$name" "$duration_ms" "$reason"
        echo "$name  FAIL  $duration_ms  $reason" >> "$RESULT_FILE"
        # Surface error context (first 3 lines, single-line).
        if [[ -s "$CURRENT_OUT_DIR/compile_err" ]]; then
            echo "      compile_err: $(head -3 "$CURRENT_OUT_DIR/compile_err" | tr '\n' ' ')"
        fi
        if [[ $run_ok -eq 0 ]] && [[ -s "$CURRENT_OUT_DIR/run_err" ]]; then
            echo "      run_err:     $(head -3 "$CURRENT_OUT_DIR/run_err" | tr '\n' ' ')"
        fi
    fi

    # Per-iteration cleanup.
    cleanup
    CURRENT_OUT_DIR=""
done

# ─── Summary ─────────────────────────────────────────────────────────────
echo "=================================================="
echo "M0 baseline summary:"
echo "  TOTAL          = $TOTAL"
echo "  PASS           = $PASS"
echo "  FAIL           = $FAIL"
echo "  PASS-ONLY ⊂ PASS = $PARSE_ONLY_PASS"
echo "  TOTAL=${TOTAL}  PASS=${PASS}  FAIL=${FAIL}  PASS-ONLY=${PARSE_ONLY_PASS}"
echo "=================================================="
echo "Results written to: $RESULT_FILE"

if [[ $FAIL -gt 0 ]]; then
    echo
    echo "Failed tests:"
    for t in "${FAILED_TESTS[@]}"; do
        echo "  - $t"
    done
    exit 1
fi
