#!/bin/bash
# Build AHX player test for Linux using CMake

set -e

echo "Building AHX player test for Linux using CMake..."
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
    echo "✓ Built ahx_player_test"
    echo "Output: $(pwd)/ahx_player_test"
    echo
    echo "To test:"
    echo "  cd build-linux"
    echo "  ./ahx_player_test /path/to/file.ahx"
else
    echo "✗ Build failed"
    exit 1
fi
