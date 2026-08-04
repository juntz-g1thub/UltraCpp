#!/bin/bash
# UltraCPP codegen verification - run both the Rust reference compiler
# and the C port on each test program and byte-level-diff the emitted
# LLVM IR.
#
# Pre-requisites:
#   - `cargo build --release` has produced target/release/ultracpp
#   - `cd src-c && make` has produced src-c/build/uc_lexer
#
# Exit codes:
#   0  all test programs match byte-for-byte
#   1  at least one test program differs (or compilation failed)
#   2  pre-requisite binary missing

set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
C_BIN="${C_BIN:-$ROOT/src-c/build/uc_lexer}"
RUST_BIN="${RUST_BIN:-$ROOT/target/release/ultracpp}"

if [[ ! -x "$C_BIN" ]]; then
    echo "Error: C compiler not built. Run: cd src-c && make"
    exit 2
fi
if [[ ! -x "$RUST_BIN" ]]; then
    echo "Error: Rust compiler not built. Run: cargo build --release"
    exit 2
fi

TESTS=(
    "test/test_t1"
    "test/test_t2"
    "test/test_t3"
    "test/test_hello_world"
    "test/test_io"
)

cd "$ROOT"

ok=0
fail=0
fails_list=""

for d in "${TESTS[@]}"; do
    src="$d/main.upp"
    if [[ ! -f "$src" ]]; then
        echo "  SKIP  $src  (file not found)"
        continue
    fi

    name="$(basename "$d")"
    rust_out="/tmp/uc_rust_ll_$name.ll"
    c_out="/tmp/uc_c_ll_$name.ll"

    if ! "$RUST_BIN" "$src" -o "$rust_out" 2>/dev/null; then
        echo "  FAIL  $src  (Rust codegen error)"
        fail=$((fail + 1))
        fails_list="$fails_list\n    $src (rust error)"
        continue
    fi
    if ! "$C_BIN" --emit-ll "$src" -o "$c_out" 2>/dev/null; then
        echo "  FAIL  $src  (C codegen error)"
        fail=$((fail + 1))
        fails_list="$fails_list\n    $src (c error)"
        continue
    fi

    if diff -q "$rust_out" "$c_out" >/dev/null 2>&1; then
        n=$(wc -l < "$c_out")
        echo "  OK    $src  ($n IR lines, byte-exact)"
        ok=$((ok + 1))
    else
        echo "  FAIL  $src  (IR differs)"
        diff "$rust_out" "$c_out" | head -30
        fail=$((fail + 1))
        fails_list="$fails_list\n    $src"
    fi
done

echo
echo "=========================================="
echo "Summary: $ok passed, $fail failed"
if [[ $fail -gt 0 ]]; then
    printf "Failed:%b\n" "$fails_list"
    exit 1
fi