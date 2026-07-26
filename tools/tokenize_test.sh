#!/bin/bash
# UltraCPP C port verification - tokenize each test program with both
# the Rust reference compiler and the C lexer, then compare the outputs.
#
# Note: columns are normalized because the Rust lexer has a known bug
# where column tracking does not update during scan_identifier /
# scan_number / scan_string, so columns drift after multi-char tokens
# preceded by whitespace. Token kinds, line numbers, lexemes, and
# literal values are compared byte-for-byte.

set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
C_BIN="${C_BIN:-$ROOT/src-c/build/uc_lexer}"
RUST_BIN="${RUST_BIN:-$ROOT/target/debug/ultracpp}"

if [[ ! -x "$C_BIN" ]]; then
    echo "Error: C lexer not built. Run: cd src-c && make"
    exit 2
fi
if [[ ! -x "$RUST_BIN" ]]; then
    echo "Error: Rust compiler not built. Run: cargo build"
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

# Normalize: keep "TOKEN <kind> <line>  <lexeme>[extras]" but drop the
# "<col>" portion (chars 23..26 in the fixed-width prefix).
#
# Token layout: "TOKEN " (6) + "<kind padded 12>" (12) + " " (1) +
#               "<line padded 4>" (4) + ":" (1) + "<col padded 3>" (3) +
#               "  " (2) + "<lexeme>[extras]"
# So col occupies chars 23-25 (0-indexed), which we strip.
normalize() {
    python3 -c '
import sys
for line in sys.stdin:
    line = line.rstrip("\n")
    # Keep "TOKEN <kind> <line>" and skip the 3-char col then 2 spaces
    if len(line) >= 26:
        line = line[:23] + line[26:]
    print(line)
'
}

ok=0
fail=0
fails_list=""

for src in "${TESTS[@]}"; do
    if [[ ! -f "$src" ]]; then
        echo "  SKIP  $src  (file not found)"
        continue
    fi

    name="$(echo "$src" | tr '/' '_')"
    rust_out="/tmp/uc_rust_$name.txt"
    c_out="/tmp/uc_c_$name.txt"
    rust_norm="/tmp/uc_rust_norm_$name.txt"
    c_norm="/tmp/uc_c_norm_$name.txt"

    if ! "$RUST_BIN" --dump-tokens "$src" > "$rust_out" 2>/dev/null; then
        echo "  FAIL  $src  (Rust lexer error)"
        fail=$((fail + 1))
        fails_list="$fails_list\n    $src (rust error)"
        continue
    fi
    if ! "$C_BIN" "$src" > "$c_out" 2>/dev/null; then
        echo "  FAIL  $src  (C lexer error)"
        fail=$((fail + 1))
        fails_list="$fails_list\n    $src (c error)"
        continue
    fi

    normalize < "$rust_out" > "$rust_norm"
    normalize < "$c_out"   > "$c_norm"

    if diff -q "$rust_norm" "$c_norm" >/dev/null 2>&1; then
        n=$(wc -l < "$c_norm")
        echo "  OK    $src  ($n tokens, byte-equivalent after column normalization)"
        ok=$((ok + 1))
    else
        echo "  FAIL  $src  (token streams differ)"
        diff "$rust_norm" "$c_norm" | head -20
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