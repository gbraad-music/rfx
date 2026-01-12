#!/bin/bash
# Build deck player test for Windows using CMake and MinGW

set -e

TOOLCHAIN_FILE="$HOME/Projects/regroove/toolchain-mingw64.cmake"

if [ ! -f "$TOOLCHAIN_FILE" ]; then
    echo "Error: Toolchain file not found at $TOOLCHAIN_FILE"
    echo "Please ensure the regroove project is cloned at ~/Projects/regroove"
    exit 1
fi

echo "Building deck player test for Windows using CMake..."
echo

# Create build directory
mkdir -p build-windows
cd build-windows

# Configure with CMake
cmake .. \
    -DCMAKE_TOOLCHAIN_FILE="$TOOLCHAIN_FILE" \
    -DCMAKE_BUILD_TYPE=Release

# Build
make -j$(nproc)

if [ $? -eq 0 ]; then
    echo
    echo "✓ Built deckplayer.exe"
    echo "Output: $(pwd)/deckplayer.exe"
    echo
    echo "To test:"
    echo "  cd build-windows"
    echo "  ./deckplayer.exe /path/to/file.mod"
else
    echo "✗ Build failed"
    exit 1
fi
