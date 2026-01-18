#!/bin/bash
# Build Regroove oscillator units for MicroKorg2 using logue SDK container

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
LOGUE_SDK_PATH="$HOME/Projects/logue-sdk"
PLATFORM_PATH="$LOGUE_SDK_PATH/platform"
CONTAINER_IMAGE="ghcr.io/gbraad-logue/logue-sdk/dev-env:latest"

echo "========================================"
echo "Building Regroove Oscillators for MicroKorg2"
echo "========================================"
echo "Platform: $PLATFORM_PATH"
echo "Units: $SCRIPT_DIR"
echo "Container: $CONTAINER_IMAGE"
echo ""

# Check paths
if [ ! -d "$PLATFORM_PATH/microkorg2" ]; then
    echo "Error: MicroKorg2 platform not found at $PLATFORM_PATH/microkorg2"
    exit 1
fi

# Create bin directory in project root
mkdir -p "$SCRIPT_DIR/../bin"

# MicroKorg2 oscillators - default list
OSCS="rgsfz_mk2"

# If arguments provided, build only those oscillators
if [ $# -gt 0 ]; then
    OSCS="$@"
fi

for osc in $OSCS; do
    echo ""
    echo "========================================"
    echo "Building $osc..."
    echo "========================================"

    # Build using the SDK's cmd_entry pattern
    # Mount project within microkorg2 directory structure
    podman run --rm \
        -v "$PLATFORM_PATH:/workspace:rw" \
        -v "$SCRIPT_DIR/$osc:/workspace/microkorg2/$osc:rw" \
        -v "$SCRIPT_DIR/..:/rfx:ro" \
        -h logue-sdk \
        $CONTAINER_IMAGE \
        /app/cmd_entry bash -c "
            source /app/microkorg2/environment && \
            cd /workspace/microkorg2/$osc && \
            make clean SYNTH_PATH=/rfx/synth && \
            make install SYNTH_PATH=/rfx/synth
        "

    if [ $? -eq 0 ]; then
        # Copy to bin directory in project root - find any .mk2unit file in the osc directory
        find "$SCRIPT_DIR/$osc" -maxdepth 2 -name "*.mk2unit" -exec cp {} "$SCRIPT_DIR/../bin/" \;
        echo "✓ $osc built successfully"
    else
        echo "✗ $osc build failed"
        exit 1
    fi
done

echo ""
echo "========================================"
echo "All MicroKorg2 oscillators built successfully!"
echo "========================================"
echo ""

# List built files in bin/
echo "Built oscillators in ~/Projects/rfx/bin/:"
ls -lh "$SCRIPT_DIR/../bin"/*.mk2unit 2>/dev/null | awk '{print "  " $9 " (" $5 ")"}'

echo ""
echo "To load onto MicroKorg2:"
echo "  1. Connect MicroKorg2 to computer via USB"
echo "  2. Use logue-cli or Korg Kontrol Editor"
echo "  3. Load from: ~/Projects/rfx/bin/"
echo ""
