#!/bin/bash
# Build all Regroove effects for logue SDK

set -e

# Check if TOOLSDIR is set
if [ -z "$TOOLSDIR" ]; then
    echo "Error: TOOLSDIR environment variable not set"
    echo "Please set it to your logue SDK path:"
    echo "  export TOOLSDIR=/path/to/logue-sdk"
    exit 1
fi

# Check if logue SDK exists
if [ ! -d "$TOOLSDIR/logue-sdk" ]; then
    echo "Error: logue SDK not found at $TOOLSDIR/logue-sdk"
    exit 1
fi

echo "Building Regroove effects for logue SDK..."
echo "SDK path: $TOOLSDIR"
echo ""

# Build each effect
EFFECTS="distortion filter eq compressor delay phaser reverb stereo_widen model1_lpf model1_hpf model1_sculpt model1_trim"

for effect in $EFFECTS; do
    echo "Building $effect..."
    cd "$effect"
    make clean
    make

    if [ $? -eq 0 ]; then
        echo "✓ $effect built successfully"
    else
        echo "✗ $effect build failed"
        exit 1
    fi

    cd ..
    echo ""
done

echo "All effects built successfully!"
echo ""
echo "Output files (.ntkdigunit):"
for effect in $EFFECTS; do
    find "$effect" -name "*.ntkdigunit" -exec echo "  {}" \;
done
echo ""
echo "To load onto your hardware:"
echo "  logue-cli load -u <effect>.ntkdigunit"
