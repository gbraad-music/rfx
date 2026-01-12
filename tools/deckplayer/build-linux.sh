#!/bin/bash
set -e

mkdir -p build-linux
cd build-linux
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)

echo ""
echo "Build complete! Binary: build-linux/deckplayer"
echo "Usage: ./build-linux/deckplayer <module_file> [seconds]"
