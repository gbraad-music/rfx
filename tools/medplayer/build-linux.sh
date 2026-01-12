#!/bin/bash
# Build MED player test for Linux using CMake

set -e

echo "Building MED player test for Linux using CMake..."
echo

# Create build directory
mkdir -p build-linux
cd build-linux

# Configure with CMake (with synth support enabled)
cmake .. \
    -DCMAKE_BUILD_TYPE=Release \
    -DMMD_SYNTH_SUPPORT=ON

# Build
make -j$(nproc)

if [ $? -eq 0 ]; then
    echo
    echo "✓ Built med_player_synth (with synth support)"
    echo "Output: $(pwd)/med_player_synth"
    echo
    echo "To test:"
    echo "  cd build-linux"
    echo "  ./med_player_synth /path/to/file.med"
else
    echo "✗ Build failed"
    exit 1
fi
