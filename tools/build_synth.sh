#!/bin/bash
# Build RG909 Python wrapper library (for testing/tweaking tools)

cd "$(dirname "$0")" || exit 1

echo "Building RG909 Python wrapper library..."
make clean
make

if [ $? -eq 0 ]; then
    echo "✓ Build successful!"
    echo ""
    echo "Library location: $(pwd)/librg909.so"
    echo "Test program: $(pwd)/test_c_kick"
    echo ""
    echo "Run test: ./test_c_kick 0.96 0.13 200"
    echo "Run interactive tweaker: python3 interactive_sweep_tweaker.py"
else
    echo "✗ Build failed!"
    exit 1
fi
