#!/bin/bash
# Build script for ahx_preset_tool

set -e

echo "Building AHX Preset Tool..."

# Try CMake build first
if command -v cmake &> /dev/null; then
    echo "Using CMake..."

    # Clean and recreate build directory
    rm -rf build
    mkdir -p build
    cd build

    cmake ..
    make

    cd ..
    echo ""
    echo "✓ Built: build/ahx_preset_tool"
else
    # Fallback to direct gcc compilation
    echo "Using direct gcc compilation..."
    gcc -o ahx_preset_tool \
        ahx_preset_tool.c \
        ../../synth/ahx_preset.c \
        ../../synth/ahx_instrument.c \
        ../../synth/ahx_plist.c \
        ../../synth/ahx_synth_core.c \
        ../../players/tracker_voice.c \
        ../../players/tracker_modulator.c \
        -I../.. \
        -I../../synth \
        -I../../players \
        -lm \
        -Wall \
        -O2

    echo ""
    echo "✓ Built: ahx_preset_tool"
fi

echo ""
echo "Test the tool:"
echo "  ./build/ahx_preset_tool builtin 0 test.ahxp"
echo "  ./build/ahx_preset_tool info test.ahxp"
echo ""
echo "Or use the Makefile:"
echo "  make test"
