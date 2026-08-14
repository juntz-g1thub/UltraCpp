#!/bin/bash
# UltraCPP build verification - run both the Rust and C compilers all
# the way to a native executable and compare runtime outputs.
#
# For each test program in test/programs/baseline/, runs:
#   1) Rust compiler: <src> -o <name>.ll; llc + gcc -> <name>_rust
#   2) C compiler:    --build <src> -o <name>_c
# then executes both and compares exit codes + stdout.
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

# M0 baseline: m0_01 through m0_06 + m0_08 through m0_37 + m0_39 through m0_50
# (skipping m0_07 compound_assign and m0_38 typedef which were moved to M1 —
#  the C lexer has no tokens for those yet; see .dev/plans/0.3.0-borrow-check-milestones.md §3.0)
TESTS=(
    test/programs/baseline/m0_01_minimal_main
    test/programs/baseline/m0_02_int_arithmetic
    test/programs/baseline/m0_03_int_precedence
    test/programs/baseline/m0_04_comparison
    test/programs/baseline/m0_05_logical
    test/programs/baseline/m0_06_bitwise
    test/programs/baseline/m0_08_unary
    test/programs/baseline/m0_09_if_else
    test/programs/baseline/m0_10_if_else_chain
    test/programs/baseline/m0_11_nested_if
    test/programs/baseline/m0_12_while_loop
    test/programs/baseline/m0_13_for_loop
    test/programs/baseline/m0_14_nested_loops
    test/programs/baseline/m0_15_break_continue
    test/programs/baseline/m0_16_function_simple
    test/programs/baseline/m0_17_function_void
    test/programs/baseline/m0_18_function_multi_params
    test/programs/baseline/m0_19_function_recursion
    test/programs/baseline/m0_20_function_nested_calls
    test/programs/baseline/m0_21_local_global
    test/programs/baseline/m0_22_const_global
    test/programs/baseline/m0_23_string_literal
    test/programs/baseline/m0_24_string_escape
    test/programs/baseline/m0_25_string_io
    test/programs/baseline/m0_26_array_basic
    test/programs/baseline/m0_27_array_init
    test/programs/baseline/m0_28_array_index
    test/programs/baseline/m0_29_pointer_basic
    test/programs/baseline/m0_30_pointer_deref
    test/programs/baseline/m0_31_pointer_addr
    test/programs/baseline/m0_32_struct_basic
    test/programs/baseline/m0_33_struct_field
    test/programs/baseline/m0_34_alloc_free
    test/programs/baseline/m0_35_unique_keyword
    test/programs/baseline/m0_36_move_keyword
    test/programs/baseline/m0_37_function_pointer
    test/programs/baseline/m0_39_module_import
    test/programs/baseline/m0_40_module_include
    test/programs/baseline/m0_41_extern_c
    test/programs/baseline/m0_42_unsafe_block
    test/programs/baseline/m0_43_main_with_args
    test/programs/baseline/m0_44_multiple_files
    test/programs/baseline/m0_45_const_expr
    test/programs/baseline/m0_46_global_init
    test/programs/baseline/m0_47_increment_decrement
    test/programs/baseline/m0_48_ternary
    test/programs/baseline/m0_49_comments_mixed
    test/programs/baseline/m0_50_chinese_identifiers
)

cd "$ROOT"
mkdir -p "$TMP"
ok=0
fail=0
fails_list=""

for d in "${TESTS[@]}"; do
    src="$d.uc"
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