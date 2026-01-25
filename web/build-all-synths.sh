#!/bin/bash
# Build all web synths for ReGroove
# This script builds all WASM modules for the web player

set -e

echo "========================================="
echo "Building All ReGroove Web Synths"
echo "========================================="
echo

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Track successes and failures
SUCCESS_COUNT=0
FAIL_COUNT=0
FAILED_MODULES=""

# Function to build a synth with Makefile
build_with_make() {
    local dir=$1
    local name=$2

    echo -e "${YELLOW}Building $name (make web)...${NC}"

    if [ ! -d "$dir" ]; then
        echo -e "${RED}✗ Directory not found: $dir${NC}"
        ((FAIL_COUNT++))
        FAILED_MODULES="$FAILED_MODULES\n  - $name (directory not found)"
        return 1
    fi

    cd "$dir"
    if make web 2>&1 | tee /tmp/build_${name}.log; then
        echo -e "${GREEN}✓ $name built successfully${NC}"
        ((SUCCESS_COUNT++))
    else
        echo -e "${RED}✗ $name build failed${NC}"
        echo "  See /tmp/build_${name}.log for details"
        ((FAIL_COUNT++))
        FAILED_MODULES="$FAILED_MODULES\n  - $name"
    fi
    cd - > /dev/null
    echo
}

# Function to build with shell script
build_with_script() {
    local script=$1
    local name=$2

    echo -e "${YELLOW}Building $name (shell script)...${NC}"

    if [ ! -f "$script" ]; then
        echo -e "${RED}✗ Script not found: $script${NC}"
        ((FAIL_COUNT++))
        FAILED_MODULES="$FAILED_MODULES\n  - $name (script not found)"
        return 1
    fi

    if bash "$script" 2>&1 | tee /tmp/build_${name}.log; then
        echo -e "${GREEN}✓ $name built successfully${NC}"
        ((SUCCESS_COUNT++))
    else
        echo -e "${RED}✗ $name build failed${NC}"
        echo "  See /tmp/build_${name}.log for details"
        ((FAIL_COUNT++))
        FAILED_MODULES="$FAILED_MODULES\n  - $name"
    fi
    echo
}

# Check for emsdk
if ! command -v emcc &> /dev/null; then
    echo -e "${RED}ERROR: emcc not found!${NC}"
    echo "Please install and activate emsdk:"
    echo "  source ~/Applications/emsdk/emsdk_env.sh"
    echo "  OR"
    echo "  source /opt/emsdk/emsdk_env.sh"
    exit 1
fi

echo "Using emcc: $(which emcc)"
echo

# Save current directory
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
cd "$SCRIPT_DIR"

# Build synths with Makefile (make web target)
build_with_make "../plugins/RGAHX_Synth" "RGAHX_Synth"
build_with_make "../plugins/RGSID_Synth" "RGSID_Synth"
build_with_make "../plugins/RGResonate1_Synth" "RGResonate1_Synth"
build_with_make "../plugins/RG1Piano" "RG1Piano"
build_with_make "../plugins/RG909_Drum" "RG909_Drum"
build_with_make "../plugins/RGSlicer" "RGSlicer"

# Build players with shell scripts
build_with_script "../plugins/RGDeckPlayer/build-wasm.sh" "RGDeckPlayer"
build_with_script "../plugins/RGSFZ_Player/build-rgsfz-wasm.sh" "RGSFZ_Player"

# Summary
echo "========================================="
echo "Build Summary"
echo "========================================="
echo -e "${GREEN}Successful: $SUCCESS_COUNT${NC}"
if [ $FAIL_COUNT -gt 0 ]; then
    echo -e "${RED}Failed: $FAIL_COUNT${NC}"
    echo -e "${RED}Failed modules:${FAILED_MODULES}${NC}"
else
    echo -e "${GREEN}All builds successful!${NC}"
fi
echo

# List output files
echo "Output files in synths/:"
ls -lh synths/*.{js,wasm} 2>/dev/null | tail -20 || echo "  (no files found)"
echo

if [ $FAIL_COUNT -gt 0 ]; then
    exit 1
fi

echo -e "${GREEN}✓ All web synths built successfully!${NC}"
exit 0
