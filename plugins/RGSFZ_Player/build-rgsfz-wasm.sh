#!/bin/bash
# Build RGSFZ Player as WebAssembly module
# SFZ sampler with polyphonic sample playback

set -e

# Source emsdk environment
if [ -f ~/Applications/emsdk/emsdk_env.sh ]; then
    source ~/Applications/emsdk/emsdk_env.sh
elif [ -f /opt/emsdk/emsdk_env.sh ]; then
    source /opt/emsdk/emsdk_env.sh
else
    echo "ERROR: emsdk not found!"
    exit 1
fi

echo "Building RGSFZ Player WASM module..."

# Output paths
OUTDIR="../web/synths"
mkdir -p "$OUTDIR"

# Compiler flags
CFLAGS="-O3 -s WASM=1 -s ALLOW_MEMORY_GROWTH=1"
CFLAGS="$CFLAGS -s EXPORTED_RUNTIME_METHODS=[cwrap,ccall,HEAPU8,HEAP8,HEAP16,HEAPF32]"
CFLAGS="$CFLAGS -s MODULARIZE=1 -s EXPORT_ES6=1 -s EXPORT_NAME='createRGSFZModule'"
CFLAGS="$CFLAGS -I."

# Exported functions
EXPORTED_FUNCTIONS="-s EXPORTED_FUNCTIONS=[_malloc,_free"
EXPORTED_FUNCTIONS="${EXPORTED_FUNCTIONS},_rgsfz_player_create_wasm"
EXPORTED_FUNCTIONS="${EXPORTED_FUNCTIONS},_rgsfz_player_destroy_wasm"
EXPORTED_FUNCTIONS="${EXPORTED_FUNCTIONS},_rgsfz_player_load_sfz_from_memory"
EXPORTED_FUNCTIONS="${EXPORTED_FUNCTIONS},_rgsfz_player_add_region"
EXPORTED_FUNCTIONS="${EXPORTED_FUNCTIONS},_rgsfz_player_load_region_sample"
EXPORTED_FUNCTIONS="${EXPORTED_FUNCTIONS},_rgsfz_player_note_on"
EXPORTED_FUNCTIONS="${EXPORTED_FUNCTIONS},_rgsfz_player_note_off"
EXPORTED_FUNCTIONS="${EXPORTED_FUNCTIONS},_rgsfz_player_all_notes_off"
EXPORTED_FUNCTIONS="${EXPORTED_FUNCTIONS},_rgsfz_player_process_f32"
EXPORTED_FUNCTIONS="${EXPORTED_FUNCTIONS},_rgsfz_player_set_volume"
EXPORTED_FUNCTIONS="${EXPORTED_FUNCTIONS},_rgsfz_player_set_pan"
EXPORTED_FUNCTIONS="${EXPORTED_FUNCTIONS},_rgsfz_player_set_decay"
EXPORTED_FUNCTIONS="${EXPORTED_FUNCTIONS},_rgsfz_player_get_num_regions"
EXPORTED_FUNCTIONS="${EXPORTED_FUNCTIONS},_rgsfz_player_get_active_voices"
EXPORTED_FUNCTIONS="${EXPORTED_FUNCTIONS},_rgsfz_player_get_region_sample"
EXPORTED_FUNCTIONS="${EXPORTED_FUNCTIONS},_rgsfz_player_get_region_lokey"
EXPORTED_FUNCTIONS="${EXPORTED_FUNCTIONS},_rgsfz_player_get_region_hikey"
EXPORTED_FUNCTIONS="${EXPORTED_FUNCTIONS},_rgsfz_player_get_region_lovel"
EXPORTED_FUNCTIONS="${EXPORTED_FUNCTIONS},_rgsfz_player_get_region_hivel"
EXPORTED_FUNCTIONS="${EXPORTED_FUNCTIONS},_rgsfz_player_get_region_pitch"
EXPORTED_FUNCTIONS="${EXPORTED_FUNCTIONS},_rgsfz_create_audio_buffer"
EXPORTED_FUNCTIONS="${EXPORTED_FUNCTIONS},_rgsfz_destroy_audio_buffer"
EXPORTED_FUNCTIONS="${EXPORTED_FUNCTIONS}]"

# Source files
SOURCES="rgsfz_player_wasm.c"
SOURCES="$SOURCES sfz_parser.c"
SOURCES="$SOURCES synth_sample_player.c"
SOURCES="$SOURCES synth_envelope.c"

# Compile
echo "Compiling..."
emcc $CFLAGS $EXPORTED_FUNCTIONS $SOURCES -o "$OUTDIR/rgsfz-player.js"

echo "✓ Built: $OUTDIR/rgsfz-player.js"
echo "✓ Built: $OUTDIR/rgsfz-player.wasm"
echo ""
echo "This module provides:"
echo "  - SFZ file parsing"
echo "  - Polyphonic sample playback (16 voices)"
echo "  - MIDI note/velocity mapping"
echo "  - Pitch shifting and velocity layers"
echo "Done!"
