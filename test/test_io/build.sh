#!/bin/bash
set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
ULTRACPP_BIN="$PROJECT_ROOT/target/debug/ultracpp"
BUILD_DIR="$PROJECT_ROOT/build/linux/test_io"
LIB_IO_LL="$PROJECT_ROOT/lib/io.ll"

mkdir -p "$BUILD_DIR"

echo "=== UltraCPP I/O Test ==="
echo ""

echo "[1/4] Compile main.upp to IR..."
$ULTRACPP_BIN "$SCRIPT_DIR/main.upp" -o "$BUILD_DIR/main.ll"

echo "[2/4] Combine with io.ll..."
cat "$LIB_IO_LL" "$BUILD_DIR/main.ll" > "$BUILD_DIR/combined.ll"

echo "[3/4] Compile combined IR to assembly..."
llc "$BUILD_DIR/combined.ll" -o "$BUILD_DIR/test_io.s"

echo "[4/4] Link and create executable..."
gcc "$BUILD_DIR/test_io.s" -o "$BUILD_DIR/test_io"

echo ""
echo "Done: $BUILD_DIR/test_io"
