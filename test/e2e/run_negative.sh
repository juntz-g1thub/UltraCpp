#!/usr/bin/env bash
# run_negative.sh — Negative test runner (fail/ directory)
#
# Spec: .dev/plans/0.3.0-borrow-check-milestones.md §4.2 (template)
#
# STATUS (M0): placeholder implementation.
#   M0 has NO negative tests (per docs/test-outline.md §1 + §3.0 — M0 is
#   a baseline pass-run, not an error-rejection suite). The first
#   negative test is m1_n1_ampmut_rejected.uc in M1.
#   This script implements the §4.2 contract so M1 can drop it in
#   without further work, but bails out cleanly when called against
#   the current M0 setup.
#
# Usage (M1+):
#   bash test/e2e/run_negative.sh test/programs/m1_lexparse/fail
#
# Exit codes:
#   0  all expected rejections produced the right diagnostic
#   1  one or more unexpected pass/fail
#   2  preconditions not met (no fail/ dir, compiler missing, ...)

set -euo pipefail

UC="src-c/build/uc_lexer"
TEST_DIR="${1:-}"

# ─── No-directory branch (M0 state) ──────────────────────────────────────
if [[ -z "$TEST_DIR" ]]; then
    cat >&2 <<EOF
run_negative.sh: no negative-test directory supplied.

  M0 currently has no negative tests (see docs/test-outline.md §3).
  First activation expected in M1 (test/programs/m1_lexparse/fail/).

  Usage:  bash test/e2e/run_negative.sh <fail-dir>
EOF
    exit 2
fi

# ─── Preconditions ───────────────────────────────────────────────────────
if [[ ! -x "$UC" ]]; then
    echo "ERROR: compiler not built: $UC" >&2
    echo "  Fix: make -C src-c" >&2
    exit 2
fi
if [[ ! -d "$TEST_DIR" ]]; then
    echo "ERROR: directory not found: $TEST_DIR" >&2
    exit 2
fi

# Negative-test contract (per §4.2):
#   For each <name>.uc in the fail/ directory:
#     1. uc_lexer --build must REJECT the file (non-zero exit code).
#     2. The compiler's stderr must mention the diagnostic named in
#        the file's leading "// expect: <ErrorName>" comment (if any).
#        If the comment is absent, any rejection counts as a pass.

# ─── Cleanup trap ────────────────────────────────────────────────────────
CURRENT_OUT=""
cleanup() {
    if [[ -n "$CURRENT_OUT" ]] && [[ -f "$CURRENT_OUT" ]]; then
        rm -f "$CURRENT_OUT"
    fi
}
trap cleanup EXIT

# ─── Counters ────────────────────────────────────────────────────────────
PASS=0
FAIL=0
FAILED_TESTS=()

# ─── Main loop ───────────────────────────────────────────────────────────
for src in "$TEST_DIR"/*.uc; do
    [[ -e "$src" ]] || continue
    name=$(basename "$src" .uc)

    # Extract // expect: <ErrorName> from leading comments (POSIX sed).
    expected=$(grep -E '^//\s*expect:\s*[A-Za-z0-9_]+' "$src" 2>/dev/null \
        | head -1 \
        | sed -E 's|^//\s*expect:\s*([A-Za-z0-9_]+).*|\1|' \
        || true)
    [[ -z "${expected:-}" ]] && expected=""

    CURRENT_OUT=$(mktemp)

    # Capture only the compile result; redirect both stdout and stderr.
    compile_failed=0
    if "$UC" --build "$src" -o /dev/null \
            2>"$CURRENT_OUT" >/dev/null; then
        compile_failed=0
    else
        compile_failed=1
    fi

    if [[ $compile_failed -eq 0 ]]; then
        echo "✗ $name: expected to fail but compiled cleanly"
        FAIL=$((FAIL + 1))
        FAILED_TESTS+=("$name")
    else
        if [[ -z "$expected" ]] || grep -qF "$expected" "$CURRENT_OUT"; then
            if [[ -n "$expected" ]]; then
                echo "✓ $name (rejected with $expected)"
            else
                echo "✓ $name (rejected)"
            fi
            PASS=$((PASS + 1))
        else
            echo "⬜ $name: rejected but expected '$expected' not in stderr"
            FAIL=$((FAIL + 1))
            FAILED_TESTS+=("$name")
            echo "    stderr: $(head -3 "$CURRENT_OUT" | tr '\n' ' ')"
        fi
    fi

    cleanup
    CURRENT_OUT=""
done

# ─── Summary ─────────────────────────────────────────────────────────────
echo "=================================================="
echo "Negative test summary:  PASS=$PASS  FAIL=$FAIL"
echo "=================================================="

[[ $FAIL -eq 0 ]] || exit 1
