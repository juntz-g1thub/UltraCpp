#!/bin/bash
# UltraCPP C port verification - tokenize each test program with both
# the Rust reference compiler (where it supports --dump-tokens) and the
# C lexer, then diff.
#
# Currently the Rust compiler does NOT expose --dump-tokens, so this
# script just runs the C lexer on every test program and reports
# the line counts. Once Rust has --dump-tokens, uncomment the Rust
# section below.

set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
C_BIN="${C_BIN:-$ROOT/src-c/build/uc_lexer}"
RUST_BIN="${RUST_BIN:-$ROOT/target/debug/ultracpp}"

if [[ ! -x "$C_BIN" ]]; then
    echo "Error: C lexer not built. Run: cd src-c && make"
    exit 2
fi

TESTS=(
    "test/test_t1/main.upp"
    "test/test_t2/main.upp"
    "test/test_t3/main.upp"
    "test/test_hello_world/main.upp"
    "test/test_io/main.upp"
    "test/test_include/main.upp"
    "test/test_simple.uc"
)

cd "$ROOT"

ok=0
fail=0

for src in "${TESTS[@]}"; do
    if [[ ! -f "$src" ]]; then
        echo "  SKIP  $src  (file not found)"
        continue
    fi
    echo "  C     $src"
    out_c="/tmp/uc_c_tokens_$(echo "$src" | tr '/' '_').txt"
    if ! "$C_BIN" "$src" > "$out_c" 2>/dev/null; then
        echo "  FAIL  $src  (C lexer error)"
        fail=$((fail + 1))
        continue
    fi
    c_lines=$(wc -l < "$out_c")
    echo "        -> $c_lines tokens"
    ok=$((ok + 1))
done

echo
echo "Summary: $ok passed, $fail failed"

if [[ $fail -gt 0 ]]; then
    exit 1
fi