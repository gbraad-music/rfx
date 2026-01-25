#!/bin/bash

# Build RGAHX Drum for WASM

echo "Building RGAHX Drum for WASM..."

# Emscripten compiler
EMCC=${EMSCRIPTEN:-emcc}

# Source files
SOURCES="
rgahxdrum.c
../../synth/ahx_preset.c
../../synth/ahx_instrument.c
../../synth/ahx_synth_core.c
../../synth/ahx_plist.c
../../synth/ahx_waves.c
../../players/tracker_voice.c
../../players/tracker_modulator.c
"

# Output files
OUTPUT_JS="../../web/synths/rgahxdrum.js"
OUTPUT_WASM="../../web/synths/rgahxdrum.wasm"

# Compile with Emscripten
$EMCC $SOURCES \
    -I../../synth \
    -I../../players \
    -O3 \
    -s WASM=1 \
    -s EXPORTED_FUNCTIONS='["_rgahxdrum_create","_rgahxdrum_destroy","_rgahxdrum_trigger","_rgahxdrum_process","_malloc","_free"]' \
    -s EXPORTED_RUNTIME_METHODS='["ccall","cwrap"]' \
    -s MODULARIZE=1 \
    -s EXPORT_NAME="RGAHXDrumModule" \
    -s ALLOW_MEMORY_GROWTH=1 \
    -s INITIAL_MEMORY=33554432 \
    -s MAXIMUM_MEMORY=67108864 \
    -s STACK_SIZE=2097152 \
    -o $OUTPUT_JS

if [ $? -eq 0 ]; then
    echo "✅ Build successful!"
    echo "   JS: $OUTPUT_JS"
    echo "   WASM: $OUTPUT_WASM"
else
    echo "❌ Build failed"
    exit 1
fi
