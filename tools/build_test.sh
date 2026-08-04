#!/bin/bash
# UltraCPP build verification - run both the Rust and C compilers all
# the way to a native executable and compare runtime outputs.
#
# For each test program in test/, runs:
#   1) Rust compiler: <src> -o <name>.ll; llc + gcc -> <name>_rust
#   2) C compiler:    --build <src> -o <name>_c
# then executes both and compares exit codes + stdout.
#
# Tests that need an external `io` module (test_hello_world / test_io)
# are expected to FAIL at the link step because the real libio isn't
# present in the repo. They are skipped here.
#
# Pre-requisites:
#   - `cargo build --release` produced target/release/ultracpp
#   - `cd src-c && make` produced src-c/build/uc_lexer
#   - `llc` and `gcc` available on PATH

set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
C_BIN="${C_BIN:-$ROOT/src-c/build/uc_lexer}"
RUST_BIN="${RUST_BIN:-$ROOT/target/release/ultracpp}"
TMP="${TMPDIR:-/tmp}/uc_build_test"

if [[ ! -x "$C_BIN" ]]; then
    echo "Error: C compiler not built. Run: cd src-c && make"
    exit 2
fi
if [[ ! -x "$RUST_BIN" ]]; then
    echo "Error: Rust compiler not built. Run: cargo build --release"
    exit 2
fi

# Tests that DO NOT need libio - safe to build & run end-to-end.
TESTS=(
    "test/test_t1"
    "test/test_t2"
    "test/test_t3"
)

cd "$ROOT"
mkdir -p "$TMP"
ok=0
fail=0
fails_list=""

for d in "${TESTS[@]}"; do
    src="$d/main.upp"
    name="$(basename "$d")"
    echo "=== $name ==="

    # 1) Rust: lex + parse + codegen, then llc + gcc.
    if ! "$RUST_BIN" "$src" -o "$TMP/$name.ll" 2>/dev/null; then
        echo "  FAIL  rust codegen"
        fail=$((fail + 1)); fails_list="$fails_list\n    $name (rust codegen)"; continue
    fi
    if ! llc "$TMP/$name.ll" -o "$TMP/$name.s" 2>/dev/null; then
        echo "  FAIL  llc"
        fail=$((fail + 1)); fails_list="$fails_list\n    $name (llc)"; continue
    fi
    if ! gcc -no-pie "$TMP/$name.s" -o "$TMP/${name}_rust" 2>/dev/null; then
        echo "  FAIL  gcc-link (rust)"
        fail=$((fail + 1)); fails_list="$fails_list\n    $name (rust gcc)"; continue
    fi

    # 2) C port: --build does everything end-to-end.
    if ! "$C_BIN" --build "$src" -o "$TMP/${name}_c" 2>/dev/null; then
        echo "  FAIL  c build"
        fail=$((fail + 1)); fails_list="$fails_list\n    $name (c build)"; continue
    fi

    # 3) Run both and compare exit codes.
    rust_exit=$("$TMP/${name}_rust"; echo $?)
    c_exit=$("$TMP/${name}_c"; echo $?)
    if [[ "$rust_exit" == "$c_exit" ]]; then
        echo "  OK    $name  (exit $c_exit matches Rust)"
        ok=$((ok + 1))
    else
        echo "  FAIL  $name  (rust=$rust_exit  c=$c_exit)"
        fail=$((fail + 1)); fails_list="$fails_list\n    $name (exit mismatch)"
    fi
done

echo
echo "=========================================="
echo "Summary: $ok passed, $fail failed"
if [[ $fail -gt 0 ]]; then
    printf "Failed:%b\n" "$fails_list"
    exit 1
fi