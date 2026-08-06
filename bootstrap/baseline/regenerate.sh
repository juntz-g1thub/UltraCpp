#!/bin/bash
# Regenerate the test_t1 baseline outputs.
#
# This script:
#   1. Compiles test_t1 with the C port (host compiler)
#   2. Compiles test_t1 with the Rust port (reference compiler)
#   3. Saves the .ll and .s outputs for the new UltraCPP compiler
#      (src-uc/) to byte-exactly match.
#
# Pre-requisites:
#   - src-c/ built:  cd src-c && make
#   - Rust built:     cargo build --release
#   - llc and gcc on PATH
#
# After running, this dir should contain:
#   test_t1.c.ll     - canonical target IR (C port output)
#   test_t1.c.s      - canonical target x86-64 assembly
#   test_t1.rust.ll  - reference IR (Rust port output, byte-exact to .c.ll)
#   test_t1.rust.s   - reference x86-64 assembly
#
# The .c.ll and .c.s are the "target" the new UltraCPP compiler must
# produce byte-exactly when compiling test/test_t1/main.upp.

set -euo pipefail

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
cd "$ROOT"

mkdir -p bootstrap/baseline

# C port
src-c/build/uc_lexer --emit-ll test/test_t1/main.upp -o bootstrap/baseline/test_t1.c.ll
llc bootstrap/baseline/test_t1.c.ll -o bootstrap/baseline/test_t1.c.s

# Rust port
./target/release/ultracpp test/test_t1/main.upp -o bootstrap/baseline/test_t1.rust.ll
llc bootstrap/baseline/test_t1.rust.ll -o bootstrap/baseline/test_t1.rust.s

# Verify
if diff -q bootstrap/baseline/test_t1.c.ll bootstrap/baseline/test_t1.rust.ll >/dev/null; then
    echo "OK  C and Rust IR are byte-exact"
else
    echo "FAIL  C and Rust IR differ"
    exit 1
fi

echo "Done.  Bootstrap baseline saved to bootstrap/baseline/."
ls -la bootstrap/baseline/
