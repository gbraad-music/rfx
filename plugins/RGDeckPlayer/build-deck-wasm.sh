#!/bin/bash
# Build Deck Player (MOD/MED/AHX) as WebAssembly module
# Uses unified deck_player abstraction for automatic format detection

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

echo "Building Deck Player WASM module..."

# Output paths
OUTDIR="../../web/players"
mkdir -p "$OUTDIR"

# Compiler flags
CFLAGS="-O3 -s WASM=1 -s ALLOW_MEMORY_GROWTH=1"
CFLAGS="$CFLAGS -s EXPORTED_RUNTIME_METHODS=[cwrap,ccall,HEAPU8,HEAP8,HEAPF32]"
CFLAGS="$CFLAGS -s MODULARIZE=1 -s EXPORT_ES6=1 -s EXPORT_NAME='createDeckPlayerModule'"
CFLAGS="$CFLAGS -I../../players -I../../synth"

# Enable MMD synth support
CFLAGS="$CFLAGS -DMMD_SYNTH_SUPPORT"

# Exported functions from deck_player_wasm.c
EXPORTED_FUNCTIONS="-s EXPORTED_FUNCTIONS=[_malloc,_free"
EXPORTED_FUNCTIONS="${EXPORTED_FUNCTIONS},_deck_player_create_wasm"
EXPORTED_FUNCTIONS="${EXPORTED_FUNCTIONS},_deck_player_destroy_wasm"
EXPORTED_FUNCTIONS="${EXPORTED_FUNCTIONS},_deck_player_load_from_memory"
EXPORTED_FUNCTIONS="${EXPORTED_FUNCTIONS},_deck_player_start_wasm"
EXPORTED_FUNCTIONS="${EXPORTED_FUNCTIONS},_deck_player_stop_wasm"
EXPORTED_FUNCTIONS="${EXPORTED_FUNCTIONS},_deck_player_is_playing_wasm"
EXPORTED_FUNCTIONS="${EXPORTED_FUNCTIONS},_deck_player_process_f32"
EXPORTED_FUNCTIONS="${EXPORTED_FUNCTIONS},_deck_player_set_channel_mute_wasm"
EXPORTED_FUNCTIONS="${EXPORTED_FUNCTIONS},_deck_player_get_channel_mute_wasm"
EXPORTED_FUNCTIONS="${EXPORTED_FUNCTIONS},_deck_player_get_current_order"
EXPORTED_FUNCTIONS="${EXPORTED_FUNCTIONS},_deck_player_get_current_pattern"
EXPORTED_FUNCTIONS="${EXPORTED_FUNCTIONS},_deck_player_get_current_row"
EXPORTED_FUNCTIONS="${EXPORTED_FUNCTIONS},_deck_player_set_position_wasm"
EXPORTED_FUNCTIONS="${EXPORTED_FUNCTIONS},_deck_player_next_pattern"
EXPORTED_FUNCTIONS="${EXPORTED_FUNCTIONS},_deck_player_prev_pattern"
EXPORTED_FUNCTIONS="${EXPORTED_FUNCTIONS},_deck_player_set_loop_pattern"
EXPORTED_FUNCTIONS="${EXPORTED_FUNCTIONS},_deck_player_get_song_length_wasm"
EXPORTED_FUNCTIONS="${EXPORTED_FUNCTIONS},_deck_player_get_num_channels_wasm"
EXPORTED_FUNCTIONS="${EXPORTED_FUNCTIONS},_deck_player_get_bpm_wasm"
EXPORTED_FUNCTIONS="${EXPORTED_FUNCTIONS},_deck_player_set_bpm_wasm"
EXPORTED_FUNCTIONS="${EXPORTED_FUNCTIONS},_deck_player_get_filename_wasm"
EXPORTED_FUNCTIONS="${EXPORTED_FUNCTIONS},_deck_player_get_type_name_wasm"
EXPORTED_FUNCTIONS="${EXPORTED_FUNCTIONS},_deck_player_get_title_wasm"
EXPORTED_FUNCTIONS="${EXPORTED_FUNCTIONS},_deck_create_audio_buffer"
EXPORTED_FUNCTIONS="${EXPORTED_FUNCTIONS},_deck_destroy_audio_buffer"
EXPORTED_FUNCTIONS="${EXPORTED_FUNCTIONS},_deck_get_buffer_size_bytes"
EXPORTED_FUNCTIONS="${EXPORTED_FUNCTIONS}]"

# Source files - unified deck player approach
SOURCES="deck_player_wasm.c"
SOURCES="$SOURCES ../../players/deck_player.c"
SOURCES="$SOURCES ../../players/mod_player.c"
SOURCES="$SOURCES ../../players/mmd_player.c"
SOURCES="$SOURCES ../../players/ahx_player.c"
SOURCES="$SOURCES ../../players/sid_player.c"
SOURCES="$SOURCES ../../players/pattern_sequencer.c"

# Shared tracker components
SOURCES="$SOURCES ../../players/tracker_mixer.c"
SOURCES="$SOURCES ../../players/tracker_voice.c"
SOURCES="$SOURCES ../../players/tracker_modulator.c"
SOURCES="$SOURCES ../../players/tracker_sequence.c"

# Synth components (for MMD, AHX, and SID)
SOURCES="$SOURCES ../../synth/synth_oscillator.c"
SOURCES="$SOURCES ../../synth/synth_envelope.c"
SOURCES="$SOURCES ../../synth/synth_lfo.c"
SOURCES="$SOURCES ../../synth/synth_sample_player.c"
SOURCES="$SOURCES ../../synth/synth_sid.c"
SOURCES="$SOURCES ../../synth/ahx_synth_core.c"
SOURCES="$SOURCES ../../synth/ahx_instrument.c"
SOURCES="$SOURCES ../../synth/ahx_preset.c"

# CPU emulation for SID
SOURCES="$SOURCES ../../common/cpu_6502.c"

# Compile
echo "Compiling..."
emcc $CFLAGS $EXPORTED_FUNCTIONS $SOURCES -o "$OUTDIR/deck-player.js"

echo "✓ Built: $OUTDIR/deck-player.js"
echo "✓ Built: $OUTDIR/deck-player.wasm"
echo ""
echo "This module supports:"
echo "  - ProTracker MOD files (automatic detection)"
echo "  - OctaMED MMD2/MMD3 files with 16-bit samples (automatic detection)"
echo "  - AHX/HVL files (automatic detection)"
echo "  - Commodore 64 SID files (automatic detection)"
echo "Done!"
