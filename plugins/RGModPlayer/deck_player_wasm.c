/*
 * WebAssembly Bindings for Deck Player (MOD/MED/AHX)
 * Unified player abstraction for all tracker formats
 */

#include <emscripten.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "../../synth/deck_player.h"

// Position callback state (stored in player wrapper)
typedef struct {
    DeckPlayer* deck;
    uint8_t current_order;
    uint16_t current_pattern;
    uint16_t current_row;
    char filename[256];
} DeckPlayerWasm;

// Position callback
static void position_callback(uint8_t order, uint16_t pattern, uint16_t row, void* user_data) {
    DeckPlayerWasm* wrapper = (DeckPlayerWasm*)user_data;
    if (wrapper) {
        wrapper->current_order = order;
        wrapper->current_pattern = pattern;
        wrapper->current_row = row;
    }
}

EMSCRIPTEN_KEEPALIVE
DeckPlayerWasm* deck_player_create_wasm(float sample_rate) {
    (void)sample_rate;  // Sample rate is passed per-process call

    DeckPlayerWasm* wrapper = (DeckPlayerWasm*)calloc(1, sizeof(DeckPlayerWasm));
    if (!wrapper) return NULL;

    wrapper->deck = deck_player_create();
    if (!wrapper->deck) {
        free(wrapper);
        return NULL;
    }

    // Set position callback to track playback position
    deck_player_set_position_callback(wrapper->deck, position_callback, wrapper);

    return wrapper;
}

EMSCRIPTEN_KEEPALIVE
void deck_player_destroy_wasm(DeckPlayerWasm* wrapper) {
    if (!wrapper) return;
    if (wrapper->deck) {
        deck_player_destroy(wrapper->deck);
    }
    free(wrapper);
}

EMSCRIPTEN_KEEPALIVE
int deck_player_load_from_memory(DeckPlayerWasm* wrapper, const uint8_t* data, size_t size, const char* filename) {
    if (!wrapper || !wrapper->deck || !data) return 0;

    bool success = deck_player_load(wrapper->deck, data, size);

    if (success && filename) {
        strncpy(wrapper->filename, filename, sizeof(wrapper->filename) - 1);
        wrapper->filename[sizeof(wrapper->filename) - 1] = '\0';
    }

    return success ? 1 : 0;
}

EMSCRIPTEN_KEEPALIVE
void deck_player_start_wasm(DeckPlayerWasm* wrapper) {
    if (wrapper && wrapper->deck) {
        deck_player_start(wrapper->deck);
    }
}

EMSCRIPTEN_KEEPALIVE
void deck_player_stop_wasm(DeckPlayerWasm* wrapper) {
    if (wrapper && wrapper->deck) {
        deck_player_stop(wrapper->deck);
    }
}

EMSCRIPTEN_KEEPALIVE
int deck_player_is_playing_wasm(const DeckPlayerWasm* wrapper) {
    if (!wrapper || !wrapper->deck) return 0;
    return deck_player_is_playing(wrapper->deck) ? 1 : 0;
}

EMSCRIPTEN_KEEPALIVE
void deck_player_process_f32(DeckPlayerWasm* wrapper, float* buffer, int frames, float sample_rate) {
    if (!wrapper || !wrapper->deck || !buffer) return;

    // Process generates: LEFT samples in buffer[0..frames-1], RIGHT in buffer[frames..2*frames-1]
    // Keep PLANAR format for ScriptProcessor compatibility
    deck_player_process(wrapper->deck, buffer, buffer + frames, frames, sample_rate);
}

EMSCRIPTEN_KEEPALIVE
void deck_player_set_channel_mute_wasm(DeckPlayerWasm* wrapper, int channel, int muted) {
    if (wrapper && wrapper->deck) {
        deck_player_set_channel_mute(wrapper->deck, channel, muted != 0);
    }
}

EMSCRIPTEN_KEEPALIVE
int deck_player_get_channel_mute_wasm(const DeckPlayerWasm* wrapper, int channel) {
    if (!wrapper || !wrapper->deck) return 0;
    return deck_player_get_channel_mute(wrapper->deck, channel) ? 1 : 0;
}

EMSCRIPTEN_KEEPALIVE
int deck_player_get_current_order(const DeckPlayerWasm* wrapper) {
    return wrapper ? wrapper->current_order : 0;
}

EMSCRIPTEN_KEEPALIVE
int deck_player_get_current_pattern(const DeckPlayerWasm* wrapper) {
    return wrapper ? wrapper->current_pattern : 0;
}

EMSCRIPTEN_KEEPALIVE
int deck_player_get_current_row(const DeckPlayerWasm* wrapper) {
    return wrapper ? wrapper->current_row : 0;
}

EMSCRIPTEN_KEEPALIVE
void deck_player_set_position_wasm(DeckPlayerWasm* wrapper, int order, int row) {
    if (wrapper && wrapper->deck) {
        deck_player_set_position(wrapper->deck, order, row);
    }
}

EMSCRIPTEN_KEEPALIVE
void deck_player_next_pattern(DeckPlayerWasm* wrapper) {
    if (!wrapper || !wrapper->deck) return;

    uint8_t len = deck_player_get_song_length(wrapper->deck);
    if (len == 0) return;

    uint8_t next = wrapper->current_order + 1;
    if (next >= len) next = 0;
    deck_player_set_position(wrapper->deck, next, 0);
}

EMSCRIPTEN_KEEPALIVE
void deck_player_prev_pattern(DeckPlayerWasm* wrapper) {
    if (!wrapper || !wrapper->deck) return;

    uint8_t len = deck_player_get_song_length(wrapper->deck);
    if (len == 0) return;

    uint8_t prev = (wrapper->current_order > 0) ? (wrapper->current_order - 1) : (len - 1);
    deck_player_set_position(wrapper->deck, prev, 0);
}

EMSCRIPTEN_KEEPALIVE
void deck_player_set_loop_pattern(DeckPlayerWasm* wrapper, int loop) {
    if (!wrapper || !wrapper->deck) return;

    if (loop) {
        // Loop current pattern only
        deck_player_set_loop_range(wrapper->deck, wrapper->current_order, wrapper->current_order);
    } else {
        // Loop entire song
        uint8_t len = deck_player_get_song_length(wrapper->deck);
        if (len > 0) {
            deck_player_set_loop_range(wrapper->deck, 0, len - 1);
        }
    }
}

EMSCRIPTEN_KEEPALIVE
int deck_player_get_song_length_wasm(const DeckPlayerWasm* wrapper) {
    if (!wrapper || !wrapper->deck) return 0;
    return deck_player_get_song_length(wrapper->deck);
}

EMSCRIPTEN_KEEPALIVE
int deck_player_get_num_channels_wasm(const DeckPlayerWasm* wrapper) {
    if (!wrapper || !wrapper->deck) return 0;
    return deck_player_get_num_channels(wrapper->deck);
}

EMSCRIPTEN_KEEPALIVE
int deck_player_get_bpm_wasm(const DeckPlayerWasm* wrapper) {
    if (!wrapper || !wrapper->deck) return 125;
    return deck_player_get_bpm(wrapper->deck);
}

EMSCRIPTEN_KEEPALIVE
void deck_player_set_bpm_wasm(DeckPlayerWasm* wrapper, int bpm) {
    if (wrapper && wrapper->deck) {
        deck_player_set_bpm(wrapper->deck, bpm);
    }
}

EMSCRIPTEN_KEEPALIVE
const char* deck_player_get_filename_wasm(const DeckPlayerWasm* wrapper) {
    return wrapper ? wrapper->filename : "";
}

EMSCRIPTEN_KEEPALIVE
const char* deck_player_get_type_name_wasm(const DeckPlayerWasm* wrapper) {
    if (!wrapper || !wrapper->deck) return "None";
    return deck_player_get_type_name(wrapper->deck);
}

EMSCRIPTEN_KEEPALIVE
const char* deck_player_get_title_wasm(const DeckPlayerWasm* wrapper) {
    if (!wrapper || !wrapper->deck) return "";
    const char* title = deck_player_get_title(wrapper->deck);
    return title ? title : "";
}

// Audio buffer helpers
EMSCRIPTEN_KEEPALIVE
void* deck_create_audio_buffer(int frames) {
    return malloc(frames * 2 * sizeof(float));
}

EMSCRIPTEN_KEEPALIVE
void deck_destroy_audio_buffer(void* buffer) {
    free(buffer);
}

EMSCRIPTEN_KEEPALIVE
int deck_get_buffer_size_bytes(int frames) {
    return frames * 2 * sizeof(float);
}
