#!/bin/bash
# Build MOD player test for Linux using CMake

set -e

echo "Building MOD player test for Linux using CMake..."
echo

# Create build directory
mkdir -p build-linux
cd build-linux

# Configure with CMake
cmake .. \
    -DCMAKE_BUILD_TYPE=Release

# Build
make -j$(nproc)

if [ $? -eq 0 ]; then
    echo
    echo "✓ Built mod_player_test"
    echo "Output: $(pwd)/mod_player_test"
    echo
    echo "To test:"
    echo "  cd build-linux"
    echo "  ./mod_player_test /path/to/file.mod"
else
    echo "✗ Build failed"
    exit 1
fi
