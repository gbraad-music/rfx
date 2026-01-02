#!/bin/bash
# Build Regroove drum synth units for Drumlogue using logue SDK container

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
LOGUE_SDK_PATH="$HOME/Projects/logue-sdk"
PLATFORM_PATH="$LOGUE_SDK_PATH/platform"
CONTAINER_IMAGE="ghcr.io/gbraad-logue/logue-sdk/dev-env:latest"

echo "========================================"
echo "Building Regroove Drum Synths for Drumlogue"
echo "========================================"
echo "Platform: $PLATFORM_PATH"
echo "Drums: $SCRIPT_DIR"
echo "Container: $CONTAINER_IMAGE"
echo ""

# Check paths
if [ ! -d "$PLATFORM_PATH/drumlogue" ]; then
    echo "Error: Drumlogue platform not found at $PLATFORM_PATH/drumlogue"
    exit 1
fi

# Create bin directory in project root
mkdir -p "$SCRIPT_DIR/../bin"

# Drumlogue drum synths - default list
DRUMS="rg909_drum"

# If arguments provided, build only those drums
if [ $# -gt 0 ]; then
    DRUMS="$@"
fi


for drum in $DRUMS; do
    echo ""
    echo "========================================"
    echo "Building $drum..."
    echo "========================================"

    # Build using the SDK's cmd_entry pattern similar to effects
    # Mount project within drumlogue directory structure
    podman run --rm \
        -v "$PLATFORM_PATH:/workspace:rw" \
        -v "$SCRIPT_DIR/$drum:/workspace/drumlogue/$drum:rw" \
        -v "$SCRIPT_DIR/..:/rfx:ro" \
        -h logue-sdk \
        $CONTAINER_IMAGE \
        /app/cmd_entry bash -c "
            source /app/drumlogue/environment && \
            cd /workspace/drumlogue/$drum && \
            make clean SYNTH_PATH=/rfx/synth && \
            make install SYNTH_PATH=/rfx/synth
        "

    if [ $? -eq 0 ]; then
        # Copy to bin directory in project root - find any .drmlgunit file in the drum directory
        find "$SCRIPT_DIR/$drum" -maxdepth 2 -name "*.drmlgunit" -exec cp {} "$SCRIPT_DIR/../bin/" \;
        echo "✓ $drum built successfully"
    else
        echo "✗ $drum build failed"
        exit 1
    fi
done

echo ""
echo "========================================"
echo "All Drumlogue drum synths built successfully!"
echo "========================================"
echo ""

# List built files in bin/
echo "Built drum synths in ~/Projects/rfx/bin/:"
ls -lh "$SCRIPT_DIR/../bin"/*.drmlgunit 2>/dev/null | awk '{print "  " $9 " (" $5 ")"}'

echo ""
echo "To load onto Drumlogue:"
echo "  1. Connect Drumlogue to computer via USB"
echo "  2. Use logue-cli or Korg Kontrol Editor"
echo "  3. Load from: ~/Projects/rfx/bin/"
echo ""
