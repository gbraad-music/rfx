#!/bin/bash
# Build Windows VCV Rack plugin using official toolchain Docker image

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

echo "Building RFX_EQ for Windows..."

podman run --rm \
  --volume="$SCRIPT_DIR/..:/home/build/rfx" \
  ghcr.io/gbraad-vcvrack/rack-plugin-toolchain:19 \
  /bin/bash -c "
    set -e
    cd /home/build/rfx/rack/RFX_EQ

    # Set up Windows cross-compilation environment
    export PATH=/home/build/rack-plugin-toolchain/local/x86_64-w64-mingw32/bin:\$PATH
    export RACK_DIR=/home/build/rack-plugin-toolchain/Rack-SDK-win-x64
    export CC=x86_64-w64-mingw32-gcc
    export CXX=x86_64-w64-mingw32-g++
    export STRIP=x86_64-w64-mingw32-strip
    export OBJCOPY=x86_64-w64-mingw32-objcopy

    echo 'Cleaning previous build...'
    make clean

    echo 'Cleaning shared effect files...'
    rm -f /home/build/rfx/effects/*.o
    rm -f /home/build/rfx/effects/*.d

    echo 'Building plugin...'
    make dist

    echo 'Build complete!'
    ls -lh dist/
  "

echo "Plugin built successfully!"
echo "Output: $SCRIPT_DIR/RFX_EQ/dist/"
