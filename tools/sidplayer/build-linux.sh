#!/bin/bash
# Build SID player test for Linux using CMake

set -e

echo "Building SID player test for Linux using CMake..."
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
    echo "✓ Built sid_player_test"
    echo "Output: $(pwd)/sid_player_test"
    echo
    echo "To test:"
    echo "  cd build-linux"
    echo "  ./sid_player_test /path/to/file.sid"
else
    echo "✗ Build failed"
    exit 1
fi
