#!/bin/bash
# Build Windows VCV Rack plugins using official toolchain Docker image

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

# List of modules to build
MODULES=("RFX_EQ" "RM1")

# If a module name is provided as argument, build only that module
if [ $# -gt 0 ]; then
    MODULES=("$@")
fi

echo "Building modules: ${MODULES[@]}"

for MODULE in "${MODULES[@]}"; do
    echo ""
    echo "========================================="
    echo "Building $MODULE for Windows..."
    echo "========================================="

    podman run --rm \
      --volume="$SCRIPT_DIR/..:/home/build/rfx" \
      ghcr.io/gbraad-vcvrack/rack-plugin-toolchain:19 \
      /bin/bash -c "
        set -e
        cd /home/build/rfx/rack/$MODULE

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

    echo "Plugin $MODULE built successfully!"
    echo "Output: $SCRIPT_DIR/$MODULE/dist/"
done

echo ""
echo "========================================="
echo "All builds complete!"
echo "========================================="

# Copy all .vcvplugin files to central bin directory
echo ""
echo "Copying plugins to ../bin/ ..."
mkdir -p ../bin

for MODULE in "${MODULES[@]}"; do
    if [ -f "$MODULE/dist/$MODULE"-*.vcvplugin ]; then
        cp -v "$MODULE/dist/$MODULE"-*.vcvplugin ../bin/
    fi
done

echo ""
echo "Plugins copied to: $(cd .. && pwd)/bin/"
ls -lh ../bin/*.vcvplugin 2>/dev/null || echo "No plugins found"
