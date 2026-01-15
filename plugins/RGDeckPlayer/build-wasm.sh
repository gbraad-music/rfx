#!/bin/bash
# Build MOD/MED player as WebAssembly module

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

echo "Building MOD/MED Player WASM module..."

# Output paths
OUTDIR="../../web/players"
mkdir -p "$OUTDIR"

# Compiler flags (matching regroove-effects.wasm build)
CFLAGS="-O3 -s WASM=1 -s ALLOW_MEMORY_GROWTH=1"
CFLAGS="$CFLAGS -s EXPORTED_RUNTIME_METHODS=[cwrap,ccall,HEAPU8,HEAP8,HEAPF32]"
CFLAGS="$CFLAGS -s MODULARIZE=1 -s EXPORT_ES6=1 -s EXPORT_NAME='createModMedPlayerModule'"
CFLAGS="$CFLAGS -I../../players -I../../synth"

EXPORTED_FUNCTIONS="-s EXPORTED_FUNCTIONS=[_malloc,_free,_modmed_player_create,_modmed_player_destroy,_modmed_player_load_from_memory,_modmed_player_start,_modmed_player_stop,_modmed_player_is_playing,_modmed_player_process_f32,_modmed_player_set_channel_mute,_modmed_player_get_channel_mute,_modmed_player_get_current_order,_modmed_player_get_current_row,_modmed_player_set_position,_modmed_player_next_pattern,_modmed_player_prev_pattern,_modmed_player_set_loop_pattern,_modmed_player_get_song_length,_modmed_player_get_num_channels,_modmed_player_get_bpm,_modmed_player_set_bpm,_modmed_player_get_filename,_modmed_player_get_type_name,_modmed_create_audio_buffer,_modmed_destroy_audio_buffer,_modmed_get_buffer_size_bytes]"

# Source files
SOURCES="mod_mmd_player_wasm.c"
SOURCES="$SOURCES ../../players/mod_player.c"
SOURCES="$SOURCES ../../players/mmd_player.c"

# Shared tracker components (required by players)
SOURCES="$SOURCES ../../players/pattern_sequencer.c"
SOURCES="$SOURCES ../../players/tracker_voice.c"
SOURCES="$SOURCES ../../players/tracker_mixer.c"

# Build with MMD synth support
CFLAGS="$CFLAGS -DMMD_SYNTH_SUPPORT"
SOURCES="$SOURCES ../../synth/synth_envelope.c"

# Compile
echo "Compiling..."
emcc $CFLAGS $EXPORTED_FUNCTIONS $SOURCES -o "$OUTDIR/modmed-player.js"

echo "✓ Built: $OUTDIR/modmed-player.js"
echo "✓ Built: $OUTDIR/modmed-player.wasm"
echo "Done!"
