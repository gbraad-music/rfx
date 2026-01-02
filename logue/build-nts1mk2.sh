#!/bin/bash
# Build script for NTS-1 MKII projects

set -e

PROJECT=$1

if [ -z "$PROJECT" ]; then
    echo "Usage: $0 <project_name>"
    echo "Available projects: rg303_nts1 rg101_nts1"
    exit 1
fi

PROJECT_DIR="/mnt/c/Users/gbraad/Projects/rfx/logue/$PROJECT"
BIN_DIR="/mnt/c/Users/gbraad/Projects/rfx/bin"

if [ ! -d "$PROJECT_DIR" ]; then
    echo "Error: Project directory $PROJECT_DIR not found"
    exit 1
fi

# Create bin directory if it doesn't exist
mkdir -p "$BIN_DIR"

echo "Building $PROJECT..."

# Build and strip in one container run
podman run --rm \
    -v "/home/gbraad/Projects/logue-sdk/platform:/workspace:rw" \
    -v "$PROJECT_DIR:/workspace/nts-1_mkii/project:rw" \
    -v "/mnt/c/Users/gbraad/Projects/rfx:/rfx:ro" \
    -v "$BIN_DIR:/output:rw" \
    ghcr.io/gbraad-logue/logue-sdk/dev-env:latest \
    /app/cmd_entry bash -c "source /app/nts-1_mkii/environment && cd /workspace/nts-1_mkii/project && make clean && make && cp build/*.elf /output/${PROJECT/_nts1/}.nts1mkiiunit && /usr/bin/arm-none-eabi-strip /output/${PROJECT/_nts1/}.nts1mkiiunit"

# Check result
UNIT_NAME="${PROJECT/_nts1/}"
if [ -f "$BIN_DIR/$UNIT_NAME.nts1mkiiunit" ]; then
    echo "Built: $BIN_DIR/$UNIT_NAME.nts1mkiiunit"
    ls -lh "$BIN_DIR/$UNIT_NAME.nts1mkiiunit"
else
    echo "Error: Build failed - output file not created"
    exit 1
fi

echo "Build complete!"
