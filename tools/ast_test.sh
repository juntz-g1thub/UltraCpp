#!/bin/bash
# UltraCPP AST verification - dump each test program's AST with both the
# Rust reference compiler and the C parser, then byte-level-diff the
# outputs.
#
# Unlike the token comparison (which normalizes columns), the AST dump
# has no line/column fields and is compared byte-for-byte.  Any diff
# here is a real divergence between the two parsers.
#
# Pre-requisites:
#   - `cargo build --release` has produced target/release/ultracpp
#   - `cd src-c && make` has produced src-c/build/uc_lexer
#
# Exit codes:
#   0  all test programs match
#   1  at least one test program differs (or compilation failed)
#   2  pre-requisite binary missing

set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
C_BIN="${C_BIN:-$ROOT/src-c/build/uc_lexer}"
RUST_BIN="${RUST_BIN:-$ROOT/target/release/ultracpp}"

if [[ ! -x "$C_BIN" ]]; then
    echo "Error: C parser not built. Run: cd src-c && make"
    exit 2
fi
if [[ ! -x "$RUST_BIN" ]]; then
    echo "Error: Rust compiler not built. Run: cargo build --release"
    exit 2
fi

TESTS=(
    "test/test_t1/main.upp"
    "test/test_t2/main.upp"
    "test/test_t3/main.upp"
    "test/test_hello_world/main.upp"
    "test/test_io/main.upp"
)

cd "$ROOT"

ok=0
fail=0
fails_list=""

for src in "${TESTS[@]}"; do
    if [[ ! -f "$src" ]]; then
        echo "  SKIP  $src  (file not found)"
        continue
    fi

    name="$(echo "$src" | tr '/' '_')"
    rust_out="/tmp/uc_rust_ast_$name.txt"
    c_out="/tmp/uc_c_ast_$name.txt"

    if ! "$RUST_BIN" --dump-ast "$src" > "$rust_out" 2>/dev/null; then
        echo "  FAIL  $src  (Rust parser error)"
        fail=$((fail + 1))
        fails_list="$fails_list\n    $src (rust error)"
        continue
    fi
    if ! "$C_BIN" --ast "$src" > "$c_out" 2>/dev/null; then
        echo "  FAIL  $src  (C parser error)"
        fail=$((fail + 1))
        fails_list="$fails_list\n    $src (c error)"
        continue
    fi

    if diff -q "$rust_out" "$c_out" >/dev/null 2>&1; then
        n=$(wc -l < "$c_out")
        echo "  OK    $src  ($n AST lines, byte-exact)"
        ok=$((ok + 1))
    else
        echo "  FAIL  $src  (AST differs)"
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