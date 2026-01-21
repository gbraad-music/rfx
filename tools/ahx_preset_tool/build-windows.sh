#!/bin/bash
# Build script for Windows cross-compilation using MinGW

set -e

echo "Building AHX Preset Tool for Windows..."
echo ""

# Check if MinGW is available
if ! command -v x86_64-w64-mingw32-gcc &> /dev/null; then
    echo "Error: MinGW-w64 cross-compiler not found"
    echo "Install it with:"
    echo "  Ubuntu/Debian: sudo apt install mingw-w64"
    echo "  Arch: sudo pacman -S mingw-w64-gcc"
    exit 1
fi

# Create build directory
mkdir -p build-windows
cd build-windows

# Configure with CMake for Windows
if command -v cmake &> /dev/null; then
    echo "Using CMake for Windows build..."
    cmake .. \
        -DCMAKE_SYSTEM_NAME=Windows \
        -DCMAKE_C_COMPILER=x86_64-w64-mingw32-gcc \
        -DCMAKE_RC_COMPILER=x86_64-w64-mingw32-windres \
        -DCMAKE_FIND_ROOT_PATH=/usr/x86_64-w64-mingw32 \
        -DCMAKE_FIND_ROOT_PATH_MODE_PROGRAM=NEVER \
        -DCMAKE_FIND_ROOT_PATH_MODE_LIBRARY=ONLY \
        -DCMAKE_FIND_ROOT_PATH_MODE_INCLUDE=ONLY

    make

    if [ -f "ahx_preset_tool.exe" ]; then
        echo ""
        echo "✓ Built: build-windows/ahx_preset_tool.exe"
        echo ""
        echo "Test it on Windows or with Wine:"
        echo "  wine ahx_preset_tool.exe builtin 0 test.ahxp"
    fi
else
    # Fallback to direct gcc compilation
    echo "Using direct MinGW compilation..."
    cd ..

    x86_64-w64-mingw32-gcc -o ahx_preset_tool.exe \
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
        -O2 \
        -static-libgcc \
        -mconsole

    if [ -f "ahx_preset_tool.exe" ]; then
        echo ""
        echo "✓ Built: ahx_preset_tool.exe"
        echo ""
        echo "Test it on Windows or with Wine:"
        echo "  wine ahx_preset_tool.exe builtin 0 test.ahxp"
    fi
fi

echo ""
echo "Transfer to Windows and test with:"
echo "  ahx_preset_tool.exe list \"C:\\Modules\\AHX\\Downstream.ahx\""
echo "  ahx_preset_tool.exe extract \"C:\\Modules\\AHX\\Downstream.ahx\" 1 bass.ahxp"
