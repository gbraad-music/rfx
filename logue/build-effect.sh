#!/bin/bash
# Build all Regroove effects for NTS-3 kaoss pad using logue SDK container

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
LOGUE_SDK_PATH="$HOME/Projects/logue-sdk"
PLATFORM_PATH="$LOGUE_SDK_PATH/platform"
CONTAINER_IMAGE="ghcr.io/gbraad-logue/logue-sdk/dev-env:latest"

echo "========================================"
echo "Building Regroove Effects for NTS-3"
echo "========================================"
echo "Platform: $PLATFORM_PATH"
echo "Effects: $SCRIPT_DIR"
echo "Container: $CONTAINER_IMAGE"
echo ""

# Check paths
if [ ! -d "$PLATFORM_PATH/nts-3_kaoss" ]; then
    echo "Error: NTS-3 platform not found at $PLATFORM_PATH/nts-3_kaoss"
    exit 1
fi

# Create bin directory in project root
mkdir -p "$SCRIPT_DIR/../bin"

# NTS-3 effects - default list
EFFECTS="distortion filter eq compressor phaser reverb stereo_widen model1"

# If arguments provided, build only those effects
if [ $# -gt 0 ]; then
    EFFECTS="$@"
fi


for effect in $EFFECTS; do
    echo ""
    echo "========================================"
    echo "Building $effect..."
    echo "========================================"

    # Build using the SDK's cmd_entry pattern
    # Mount rfx at /rfx so effects can find ../param_interface.h
    # Always do clean build to avoid stale object files
    podman run --rm \
        -v "$PLATFORM_PATH:/workspace:rw" \
        -v "$SCRIPT_DIR/$effect:/workspace/project:rw" \
        -v "$SCRIPT_DIR/..:/rfx:ro" \
        -h logue-sdk \
        $CONTAINER_IMAGE \
        /app/cmd_entry bash -c "source /app/nts-3_kaoss/environment && cd /workspace/project && make clean && make install"

    if [ $? -eq 0 ]; then
        # Copy to bin directory in project root - find any .nts3unit file in the effect directory
        find "$SCRIPT_DIR/$effect" -maxdepth 2 -name "*.nts3unit" -exec cp {} "$SCRIPT_DIR/../bin/" \;
        echo "✓ $effect built successfully"
    else
        echo "✗ $effect build failed"
        exit 1
    fi
done

echo ""
echo "========================================"
echo "All NTS-3 effects built successfully!"
echo "========================================"
echo ""

# List built files in bin/
echo "Built effects in ~/Projects/rfx/bin/:"
ls -lh "$SCRIPT_DIR/../bin"/*.nts3unit 2>/dev/null | awk '{print "  " $9 " (" $5 ")"}'

echo ""
echo "To load onto NTS-3:"
echo "  1. Connect NTS-3 to computer via USB"
echo "  2. Use logue-cli or Korg Kontrol Editor"
echo "  3. Load from: ~/Projects/rfx/bin/"
echo ""
