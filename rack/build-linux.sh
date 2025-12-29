#!/bin/bash
# Build Linux VCV Rack plugin using official toolchain Docker image

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

echo "Building RFX_EQ for Linux..."

podman run --rm \
  --volume="$SCRIPT_DIR/..:/home/build/rfx" \
  ghcr.io/gbraad-vcvrack/rack-plugin-toolchain:19 \
  /bin/bash -c "
    set -e
    cd /home/build/rfx/rack/RFX_EQ

    # Set up Linux cross-compilation environment
    export PATH=/home/build/rack-plugin-toolchain/local/x86_64-ubuntu16.04-linux-gnu/bin:\$PATH
    export RACK_DIR=/home/build/rack-plugin-toolchain/Rack-SDK-lin-x64
    export CC=x86_64-ubuntu16.04-linux-gnu-gcc
    export CXX=x86_64-ubuntu16.04-linux-gnu-g++
    export STRIP=x86_64-ubuntu16.04-linux-gnu-strip
    export OBJCOPY=x86_64-ubuntu16.04-linux-gnu-objcopy

    echo 'Cleaning previous build...'
    make clean

    echo 'Cleaning shared effect files...'
    rm -f /home/build/rfx/effects/*.o
    rm -f /home/build/rfx/effects/*.d

    echo 'Compiling fx_eq.c for Linux...'
    x86_64-ubuntu16.04-linux-gnu-gcc -c -o /home/build/rfx/effects/fx_eq.c.o \
      /home/build/rfx/effects/fx_eq.c \
      -I/home/build/rfx/effects \
      -O3 -fPIC

    echo 'Building plugin...'
    make dist

    echo 'Build complete!'
    ls -lh dist/
  "

echo "Plugin built successfully!"
echo "Output: $SCRIPT_DIR/RFX_EQ/dist/"
