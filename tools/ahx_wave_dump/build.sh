#!/bin/bash
# Build AHX Wave Dump Tool for Linux using CMake

echo "Building AHX Wave Dump Tool for Linux using CMake..."

# Create build directory
mkdir -p build
cd build

# Configure
cmake .. -DCMAKE_BUILD_TYPE=Release

# Build
make

echo ""
echo "Build complete! Binary: build/ahx_wave_dump"
