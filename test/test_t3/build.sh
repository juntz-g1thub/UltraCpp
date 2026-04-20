#!/bin/bash
set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
ULTRACPP_BIN="$PROJECT_ROOT/target/debug/ultracpp"
BUILD_DIR="$PROJECT_ROOT/build/linux/test_t3"

echo "=== UltraCPP Test: test_t3 (void main) ==="
echo ""

mkdir -p "$BUILD_DIR"

echo "[1/3] Compile .upp to .ll..."
$ULTRACPP_BIN "$SCRIPT_DIR/main.upp" -o "$BUILD_DIR/main.ll"

echo "[2/3] Compile .ll to .s..."
llc "$BUILD_DIR/main.ll" -o "$BUILD_DIR/main.s"

echo "[3/3] Link to executable..."
gcc "$BUILD_DIR/main.s" -o "$BUILD_DIR/test_t3"

echo ""
echo "Done: $BUILD_DIR/test_t3"
