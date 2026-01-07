#!/bin/bash
# Build MED player test for Windows using CMake and MinGW

set -e

TOOLCHAIN_FILE="$HOME/Projects/regroove/toolchain-mingw64.cmake"

if [ ! -f "$TOOLCHAIN_FILE" ]; then
    echo "Error: Toolchain file not found at $TOOLCHAIN_FILE"
    echo "Please ensure the regroove project is cloned at ~/Projects/regroove"
    exit 1
fi

echo "Building MED player test for Windows using CMake..."
echo

# Create build directory
mkdir -p build-windows
cd build-windows

# With synth support (ENABLED)
# cmake -DMMD_SYNTH_SUPPORT=ON . && cmake --build .)

# Configure with CMake
cmake .. \
    -DCMAKE_TOOLCHAIN_FILE="$TOOLCHAIN_FILE" \
    -DCMAKE_BUILD_TYPE=Release \
    -DMMD_SYNTH_SUPPORT=ON

# Build
make -j$(nproc)

if [ $? -eq 0 ]; then
    echo
    echo "✓ Built med_player_synth.exe (with synth support)"
    echo "Output: $(pwd)/med_player_synth.exe"
    echo
    echo "To test:"
    echo "  cd build-windows"
    echo "  ./med_player_synth.exe /path/to/file.med"
else
    echo "✗ Build failed"
    exit 1
fi
