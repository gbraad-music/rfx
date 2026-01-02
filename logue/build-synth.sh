#!/bin/bash
# Build Regroove synth units for Drumlogue using logue SDK container

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
LOGUE_SDK_PATH="$HOME/Projects/logue-sdk"
PLATFORM_PATH="$LOGUE_SDK_PATH/platform"
CONTAINER_IMAGE="ghcr.io/gbraad-logue/logue-sdk/dev-env:latest"

echo "========================================"
echo "Building Regroove Synths for Drumlogue"
echo "========================================"
echo "Platform: $PLATFORM_PATH"
echo "Synths: $SCRIPT_DIR"
echo "Container: $CONTAINER_IMAGE"
echo ""

# Check paths
if [ ! -d "$PLATFORM_PATH/drumlogue" ]; then
    echo "Error: Drumlogue platform not found at $PLATFORM_PATH/drumlogue"
    exit 1
fi

# Create bin directory in project root
mkdir -p "$SCRIPT_DIR/../bin"

# Drumlogue synths - default list
SYNTHS="rg303 rg101"

# If arguments provided, build only those synths
if [ $# -gt 0 ]; then
    SYNTHS="$@"
fi


for synth in $SYNTHS; do
    echo ""
    echo "========================================"
    echo "Building $synth..."
    echo "========================================"

    # Build using the SDK's cmd_entry pattern similar to effects
    # Mount project within drumlogue directory structure
    podman run --rm \
        -v "$PLATFORM_PATH:/workspace:rw" \
        -v "$SCRIPT_DIR/$synth:/workspace/drumlogue/$synth:rw" \
        -v "$SCRIPT_DIR/..:/rfx:ro" \
        -h logue-sdk \
        $CONTAINER_IMAGE \
        /app/cmd_entry bash -c "
            source /app/drumlogue/environment && \
            cd /workspace/drumlogue/$synth && \
            make clean SYNTH_PATH=/rfx/synth && \
            make install SYNTH_PATH=/rfx/synth
        "

    if [ $? -eq 0 ]; then
        # Copy to bin directory in project root - find any .drmlgunit file in the synth directory
        find "$SCRIPT_DIR/$synth" -maxdepth 2 -name "*.drmlgunit" -exec cp {} "$SCRIPT_DIR/../bin/" \;
        echo "✓ $synth built successfully"
    else
        echo "✗ $synth build failed"
        exit 1
    fi
done

echo ""
echo "========================================"
echo "All Drumlogue synths built successfully!"
echo "========================================"
echo ""

# List built files in bin/
echo "Built synths in ~/Projects/rfx/bin/:"
ls -lh "$SCRIPT_DIR/../bin"/*.drmlgunit 2>/dev/null | awk '{print "  " $9 " (" $5 ")"}'

echo ""
echo "To load onto Drumlogue:"
echo "  1. Connect Drumlogue to computer via USB"
echo "  2. Use logue-cli or Korg Kontrol Editor"
echo "  3. Load from: ~/Projects/rfx/bin/"
echo ""
