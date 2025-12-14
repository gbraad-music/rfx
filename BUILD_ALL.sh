#!/bin/bash
# Build ALL RegrooveFX plugins - UI and DSP-only versions
# Clean naming: RFX_*.lv2 (with UI) and RFX_*_noui.lv2 (DSP-only)

set -e  # Exit on error

echo "========================================"
echo " Building RegrooveFX Plugin Suite"
echo " 6 plugins × 2 variants (UI/noui)"
echo "========================================"
echo ""

cd "$(dirname "$0")/plugins"

# List of all 6 plugins
PLUGINS=(
    "RFX_Compressor"
    "RFX_Delay"
    "RFX_Distortion"
    "RFX_EQ"
    "RFX_Filter"
    "RegrooveFX"
)

# Build function for a single plugin
build_plugin() {
    local plugin=$1
    echo "=== Building $plugin ==="
    cd "$plugin"

    # Clean previous builds
    make clean > /dev/null 2>&1 || true

    # 1. Build DEFAULT version with UI (LV2 + VST3)
    echo "  - Default version (with UI)..."
    if make NAME="${plugin}" lv2_sep vst3 > /dev/null 2>&1; then
        echo "    ✅ ${plugin}.lv2 (with UI)"
        echo "    ✅ ${plugin}.vst3 (with UI)"
    else
        echo "    ❌ FAILED: ${plugin} UI version"
        cd ..
        return 1
    fi

    # 2. Build DSP-only version (LV2 + VST3 without UI)
    echo "  - DSP-only version (no UI)..."
    if make -f Makefile.noui NAME="${plugin}_noui" lv2_dsp vst3 > /dev/null 2>&1; then
        echo "    ✅ ${plugin}_noui.lv2 (no UI)"
        echo "    ✅ ${plugin}_noui.vst3 (no UI)"
    else
        echo "    ❌ FAILED: ${plugin} DSP-only version"
        cd ..
        return 1
    fi

    cd ..
}

# Build all plugins
echo "Building Linux versions..."
echo ""

failed=0
for plugin in "${PLUGINS[@]}"; do
    if ! build_plugin "$plugin"; then
        failed=$((failed + 1))
    fi
    echo ""
done

# Summary
echo "========================================"
echo " Build Summary"
echo "========================================"
echo ""

if [ $failed -eq 0 ]; then
    echo "✅ ALL PLUGINS BUILT SUCCESSFULLY!"
    echo ""

    # Count bundles
    lv2_ui=$(ls -1d ../bin/*.lv2 2>/dev/null | grep -v "_noui" | wc -l)
    lv2_noui=$(ls -1d ../bin/*_noui.lv2 2>/dev/null | wc -l)
    vst3_ui=$(ls -1d ../bin/*.vst3 2>/dev/null | grep -v "_noui" | wc -l)
    vst3_noui=$(ls -1d ../bin/*_noui.vst3 2>/dev/null | wc -l)

    echo "Built:"
    echo "  - $lv2_ui × LV2 with UI"
    echo "  - $lv2_noui × LV2 without UI (noui)"
    echo "  - $vst3_ui × VST3 with UI"
    echo "  - $vst3_noui × VST3 without UI (noui)"
    echo ""
    echo "Total: $((lv2_ui + lv2_noui + vst3_ui + vst3_noui)) plugin bundles"

    echo ""
    echo "Plugin list:"
    ls -1d ../bin/*.lv2 ../bin/*.vst3 2>/dev/null | sed 's|../bin/||' | sort

    echo ""
    echo "Plugin sizes:"
    for plugin in "${PLUGINS[@]}"; do
        ui_lv2="../bin/${plugin}.lv2"
        noui_lv2="../bin/${plugin}_noui.lv2"
        if [ -d "$ui_lv2" ]; then
            ui_size=$(du -sh "$ui_lv2" | cut -f1)
            noui_size=$(du -sh "$noui_lv2" | cut -f1)
            echo "  ${plugin}: UI=$ui_size, noui=$noui_size"
        fi
    done
else
    echo "❌ $failed plugin(s) failed to build"
    exit 1
fi

echo ""
echo "To install for testing:"
echo "  cp -r ../bin/*.lv2 ~/.lv2/      # All LV2 plugins"
echo "  cp -r ../bin/*.vst3 ~/.vst3/    # All VST3 plugins"
echo ""
echo "  # Or install only UI versions:"
echo "  cp -r ../bin/RFX_*.lv2 ~/.lv2/ && cp -r ../bin/RegrooveFX.lv2 ~/.lv2/"
echo ""
echo "For Windows builds:"
echo "  cd plugins/<plugin> && make -f Makefile.noui WINDOWS=true NAME=<plugin>_noui vst3"
