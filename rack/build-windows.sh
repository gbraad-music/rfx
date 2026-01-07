#!/bin/bash
# Build Windows VCV Rack plugins using official toolchain Docker image

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

# List of modules to build
MODULES=("RegrooveFX" "RegrooveM1" "RegrooveDJ" "RegrooveMod" "RG909_Drum" "RG303_Synth" "RG808_Drum")

# If a module name is provided as argument, build only that module
if [ $# -gt 0 ]; then
    MODULES=("$@")
fi

echo "Building modules: ${MODULES[@]}"

# Clean shared effect and synth object files BEFORE building any module
echo ""
echo "Cleaning shared object files..."
find "$SCRIPT_DIR/../effects" -name "*.o" -delete 2>/dev/null || true
find "$SCRIPT_DIR/../effects" -name "*.d" -delete 2>/dev/null || true
find "$SCRIPT_DIR/../synth" -name "*.o" -delete 2>/dev/null || true
find "$SCRIPT_DIR/../synth" -name "*.d" -delete 2>/dev/null || true
# Also clean rack/effects and rack/synth if they exist (created by local builds)
rm -rf "$SCRIPT_DIR/effects" 2>/dev/null || true
rm -rf "$SCRIPT_DIR/synth" 2>/dev/null || true
echo "Shared files cleaned."

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

        echo 'Cleaning shared effect and synth files...'
        rm -f /home/build/rfx/effects/*.o
        rm -f /home/build/rfx/effects/*.d
        rm -f /home/build/rfx/synth/*.o
        rm -f /home/build/rfx/synth/*.d

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

# Copy .vcvplugin files to central bin directory
echo ""
echo "Copying plugins to ../bin/ ..."
mkdir -p ../bin

COPIED_COUNT=0
for MODULE in "${MODULES[@]}"; do
    if [ -f "$MODULE/dist/$MODULE"-*.vcvplugin ]; then
        cp -v "$MODULE/dist/$MODULE"-*.vcvplugin ../bin/
        COPIED_COUNT=$((COPIED_COUNT + 1))
    fi
done

echo ""
if [ $COPIED_COUNT -eq 0 ]; then
    echo "No plugins copied."
else
    echo "$COPIED_COUNT plugin(s) copied to: $(cd .. && pwd)/bin/"
fi
