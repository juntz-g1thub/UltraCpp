#!/bin/bash
# test/e2e/run_lib_tests.sh
# Lib stdlib test runner for test/programs/lib_unit/*.uc
# Pattern: compile → link → run → compare stdout (or exit code)
# Per 0.3.5 plan §3.1 commit 14g' (D7 decision)
#
# Usage:
#   bash test/e2e/run_lib_tests.sh
#   (must be run from the repository root)
#
# Tests cover 4 active stdlib files: lib/{alloc,print,memory,string}.uc
# Excludes deprecated lib/io.uc + lib/math.uc (per 0.3.6 removal plan).
#
# Test mode per entry (registry at bottom):
#   stdout-only   — pass iff `actual_stdout == expected_stdout` (with \n escapes)
#   exit-only     — pass iff `actual_exit == expected_exit`
#   both          — both checks must hold
#   skip          — log only, do not build/run (used for STUBs awaiting 14h)
#
# Exit codes:
#   0  all PASS (SKIP counts as OK)
#   1  one or more FAIL
#   2  preconditions not met (compiler missing, test dir missing, ...)

set -euo pipefail

# ─── Configuration ───────────────────────────────────────────────────────
UC_COMPILER="${UC_COMPILER:-src-c/build/uc_lexer}"
LIB_UNIT_DIR="test/programs/lib_unit"

# ─── Cleanup trap ────────────────────────────────────────────────────────
CURRENT_OUT_DIR=""
cleanup() {
    if [[ -n "$CURRENT_OUT_DIR" ]] && [[ -d "$CURRENT_OUT_DIR" ]]; then
        rm -rf "$CURRENT_OUT_DIR"
    fi
}
trap cleanup EXIT

# ─── Preconditions ───────────────────────────────────────────────────────
if [[ ! -x "$UC_COMPILER" ]]; then
    echo "ERROR: compiler not built: $UC_COMPILER" >&2
    echo "  Fix: make -C src-c" >&2
    exit 2
fi
if [[ ! -d "$LIB_UNIT_DIR" ]]; then
    echo "ERROR: test directory not found: $LIB_UNIT_DIR" >&2
    echo "  Hint: run this script from the repository root." >&2
    exit 2
fi

# ─── Counters ────────────────────────────────────────────────────────────
PASS=0
FAIL=0
SKIP=0
FAILED_TESTS=()

# ─── Test runner ─────────────────────────────────────────────────────────
# Usage: run_lib_test <uc_file> <expected_stdout> <expected_exit> <skip_flag>
#   <expected_stdout>  string with \n escapes (empty = don't check)
#   <expected_exit>    integer or "" (empty = don't check)
#   <skip_flag>        "skip" = skip without running; anything else = run
run_lib_test() {
    local uc_file="$1"
    local expected_stdout="$2"
    local expected_exit="$3"
    local skip_flag="$4"

    local test_name
    test_name=$(basename "$uc_file" .uc)

    if [[ "$skip_flag" == "skip" ]]; then
        printf "⏭  SKIP: %-30s  (placeholder; activates at 14h)\n" "$test_name"
        SKIP=$((SKIP + 1))
        return
    fi

    CURRENT_OUT_DIR=$(mktemp -d)

    local out_exe="$CURRENT_OUT_DIR/$test_name"
    local actual_stdout_file="$CURRENT_OUT_DIR/${test_name}.stdout"
    local actual_exit=-1
    local build_ok=0
    local run_ok=-1

    # ── Compile + link ──
    if "$UC_COMPILER" --build "$uc_file" -o "$out_exe" \
            >"$CURRENT_OUT_DIR/build_err.log" 2>&1; then
        build_ok=1
    fi

    # ── Run (only if compile succeeded) ──
    if [[ $build_ok -eq 1 ]] && [[ -x "$out_exe" ]]; then
        set +e
        "$out_exe" >"$actual_stdout_file" 2>&1
        actual_exit=$?
        set -e
        run_ok=1
    fi

    # ── Decide pass / fail ──
    local passed=0
    local reason=""

    if [[ $build_ok -ne 1 ]]; then
        reason="build_failed: $(tr '\n' ' ' < "$CURRENT_OUT_DIR/build_err.log" | head -c 200)"
    elif [[ $run_ok -ne 1 ]]; then
        reason="no_executable_after_build"
    else
        # stdout diff (if expected_stdout non-empty)
        if [[ -n "$expected_stdout" ]]; then
            local expected_stdout_file="$CURRENT_OUT_DIR/${test_name}.expected"
            printf "%b" "$expected_stdout" >"$expected_stdout_file"
            if ! diff -q "$expected_stdout_file" "$actual_stdout_file" >/dev/null 2>&1; then
                reason="stdout_mismatch expected=$(head -c 60 "$expected_stdout_file" | tr '\n' '|') actual=$(head -c 60 "$actual_stdout_file" | tr '\n' '|')"
            fi
        fi
        # exit code diff (if expected_exit non-empty)
        if [[ -z "$reason" ]] && [[ -n "$expected_exit" ]] && [[ "$actual_exit" != "$expected_exit" ]]; then
            reason="exit_code_mismatch expected=$expected_exit actual=$actual_exit"
        fi
        if [[ -z "$reason" ]]; then
            passed=1
        fi
    fi

    # ── Report ──
    if [[ $passed -eq 1 ]]; then
        PASS=$((PASS + 1))
        printf "✓  PASS: %-30s  exit=%s\n" "$test_name" "$actual_exit"
    else
        FAIL=$((FAIL + 1))
        FAILED_TESTS+=("$test_name")
        printf "✗  FAIL: %-30s  %s\n" "$test_name" "$reason"
    fi

    # Per-iteration cleanup.
    cleanup
    CURRENT_OUT_DIR=""
}

# ─── Test registry ──────────────────────────────────────────────────────
# (uc_file, expected_stdout, expected_exit, skip_flag)
# Format note: use literal \n inside double-quoted expected_stdout
#             (printf "%b" is used at compare time, so \n is honored).
run_lib_test "$LIB_UNIT_DIR/test_print_str.uc"   "hello\n"                           ""   ""
run_lib_test "$LIB_UNIT_DIR/test_print_num.uc"   "0\n42\n-7\n-2147483648\n"           ""   ""
run_lib_test "$LIB_UNIT_DIR/test_print_float.uc" ""                                   ""   "skip"
run_lib_test "$LIB_UNIT_DIR/test_alloc.uc"       "42\n"                              ""   ""
run_lib_test "$LIB_UNIT_DIR/test_free.uc"        ""                                   "0"  ""

# ─── Summary ─────────────────────────────────────────────────────────────
echo ""
echo "============================================"
echo "Lib Tests: PASS=$PASS FAIL=$FAIL SKIP=$SKIP"
echo "============================================"

if [[ $FAIL -gt 0 ]]; then
    echo
    echo "Failed tests:"
    for t in "${FAILED_TESTS[@]}"; do
        echo "  - $t"
    done
    exit 1
fi
exit 0
