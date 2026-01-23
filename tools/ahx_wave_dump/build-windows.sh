#!/bin/bash
# Build AHX Wave Dump Tool for Windows using CMake and MinGW cross-compiler

echo "Building AHX Wave Dump Tool for Windows using CMake..."

# Create build directory
mkdir -p build-windows
cd build-windows

# Configure with MinGW toolchain
cmake .. \
    -DCMAKE_TOOLCHAIN_FILE=../../../cmake/mingw-w64-x86_64.cmake \
    -DCMAKE_BUILD_TYPE=Release

# Build
make

echo ""
echo "Build complete! Binary: build-windows/ahx_wave_dump.exe"
