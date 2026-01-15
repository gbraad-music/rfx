/*
 * Unified Deck Player Implementation
 * Manages MOD/MED/AHX players with automatic format detection
 */

#include "deck_player.h"
#include "mod_player.h"
#include "mmd_player.h"
#include "ahx_player.h"
#include "pattern_sequencer.h"
#include <stdlib.h>
#include <string.h>

struct DeckPlayer {
    // Active player type
    DeckPlayerType type;

    // Player instances (only one active at a time)
    ModPlayer* mod_player;
    MedPlayer* med_player;
    AhxPlayer* ahx_player;

    // Position callback state
    DeckPlayerPositionCallback position_callback;
    void* position_callback_userdata;

    // Channel mute states (shared across all player types)
    bool channel_muted[4];
};

// Forward declarations for internal callbacks
static void mod_position_callback(uint8_t order, uint8_t pattern, uint16_t row, void* user_data);
static void med_position_callback(uint8_t order, uint8_t pattern, uint16_t row, void* user_data);
static void ahx_position_callback(uint8_t subsong, uint16_t position, uint16_t row, void* user_data);

DeckPlayer* deck_player_create(void) {
    DeckPlayer* player = calloc(1, sizeof(DeckPlayer));
    if (!player) return NULL;

    // Create all three player instances
    player->mod_player = mod_player_create();
    player->med_player = med_player_create();
    player->ahx_player = ahx_player_create();

    if (!player->mod_player || !player->med_player || !player->ahx_player) {
        deck_player_destroy(player);
        return NULL;
    }

    player->type = DECK_PLAYER_NONE;

    return player;
}

void deck_player_destroy(DeckPlayer* player) {
    if (!player) return;

    if (player->mod_player) mod_player_destroy(player->mod_player);
    if (player->med_player) med_player_destroy(player->med_player);
    if (player->ahx_player) ahx_player_destroy(player->ahx_player);

    free(player);
}

bool deck_player_load(DeckPlayer* player, const uint8_t* data, size_t size) {
    if (!player || !data || size == 0) return false;

    // Reset state
    player->type = DECK_PLAYER_NONE;
    memset(player->channel_muted, 0, sizeof(player->channel_muted));

    // Try each player in order until one succeeds
    bool success = false;

    // Try MOD first (most common)
    if (mod_player_detect(data, size)) {
        success = mod_player_load(player->mod_player, data, size);
        if (success) {
            player->type = DECK_PLAYER_MOD;
            mod_player_set_position_callback(player->mod_player, mod_position_callback, player);
        }
    }
    // Try MED
    else if (med_player_detect(data, size)) {
        success = med_player_load(player->med_player, data, size);
        if (success) {
            player->type = DECK_PLAYER_MED;
            med_player_set_position_callback(player->med_player, med_position_callback, player);
        }
    }
    // Try AHX
    else if (ahx_player_detect(data, size)) {
        success = ahx_player_load(player->ahx_player, data, size);
        if (success) {
            player->type = DECK_PLAYER_AHX;
            ahx_player_set_position_callback(player->ahx_player, ahx_position_callback, player);
        }
    }

    // Reset channel mutes on all players
    if (success) {
        for (int i = 0; i < 4; i++) {
            if (player->type == DECK_PLAYER_MOD) {
                mod_player_set_channel_mute(player->mod_player, i, false);
            } else if (player->type == DECK_PLAYER_MED) {
                med_player_set_channel_mute(player->med_player, i, false);
            } else if (player->type == DECK_PLAYER_AHX) {
                ahx_player_set_channel_mute(player->ahx_player, i, false);
            }
        }
    }

    return success;
}

DeckPlayerType deck_player_get_type(const DeckPlayer* player) {
    if (!player) return DECK_PLAYER_NONE;
    return player->type;
}

const char* deck_player_get_type_name(const DeckPlayer* player) {
    if (!player) return "None";

    switch (player->type) {
        case DECK_PLAYER_MOD: return "ProTracker MOD";
        case DECK_PLAYER_MED: return "OctaMED";
        case DECK_PLAYER_AHX: return "AHX/HVL";
        default: return "None";
    }
}

const char* deck_player_get_title(const DeckPlayer* player) {
    if (!player) return NULL;

    switch (player->type) {
        case DECK_PLAYER_MOD:
            return mod_player_get_title(player->mod_player);
        case DECK_PLAYER_MED:
            return NULL;  // MED player doesn't expose title
        case DECK_PLAYER_AHX:
            return ahx_player_get_title(player->ahx_player);
        default:
            return NULL;
    }
}

void deck_player_start(DeckPlayer* player) {
    if (!player) return;

    switch (player->type) {
        case DECK_PLAYER_MOD:
            mod_player_start(player->mod_player);
            break;
        case DECK_PLAYER_MED:
            med_player_start(player->med_player);
            break;
        case DECK_PLAYER_AHX:
            ahx_player_start(player->ahx_player);
            break;
        default:
            break;
    }
}

void deck_player_stop(DeckPlayer* player) {
    if (!player) return;

    switch (player->type) {
        case DECK_PLAYER_MOD:
            mod_player_stop(player->mod_player);
            break;
        case DECK_PLAYER_MED:
            med_player_stop(player->med_player);
            break;
        case DECK_PLAYER_AHX:
            ahx_player_stop(player->ahx_player);
            break;
        default:
            break;
    }
}

bool deck_player_is_playing(const DeckPlayer* player) {
    if (!player) return false;

    switch (player->type) {
        case DECK_PLAYER_MOD:
            return mod_player_is_playing(player->mod_player);
        case DECK_PLAYER_MED:
            return med_player_is_playing(player->med_player);
        case DECK_PLAYER_AHX:
            return ahx_player_is_playing(player->ahx_player);
        default:
            return false;
    }
}

void deck_player_set_position_callback(DeckPlayer* player, DeckPlayerPositionCallback callback, void* user_data) {
    if (!player) return;
    player->position_callback = callback;
    player->position_callback_userdata = user_data;
}

void deck_player_get_position(const DeckPlayer* player, uint8_t* order, uint16_t* pattern, uint16_t* row) {
    if (!player) return;

    switch (player->type) {
        case DECK_PLAYER_MOD: {
            uint8_t pat8;
            mod_player_get_position(player->mod_player, &pat8, row);
            if (order) *order = 0;  // MOD doesn't expose order separately
            if (pattern) *pattern = pat8;
            break;
        }
        case DECK_PLAYER_MED: {
            uint8_t pat8;
            med_player_get_position(player->med_player, &pat8, row);
            if (order) *order = 0;  // MED doesn't expose order separately
            if (pattern) *pattern = pat8;
            break;
        }
        case DECK_PLAYER_AHX:
            ahx_player_get_position(player->ahx_player, pattern, row);
            if (order) *order = 0;  // AHX doesn't expose subsong in position
            break;
        default:
            if (order) *order = 0;
            if (pattern) *pattern = 0;
            if (row) *row = 0;
            break;
    }
}

void deck_player_set_position(DeckPlayer* player, uint8_t order, uint16_t row) {
    if (!player) return;

    switch (player->type) {
        case DECK_PLAYER_MOD:
            mod_player_set_position(player->mod_player, order, row);
            break;
        case DECK_PLAYER_MED:
            med_player_set_position(player->med_player, order, row);
            break;
        case DECK_PLAYER_AHX:
            // AHX doesn't have set_position API
            break;
        default:
            break;
    }
}

uint8_t deck_player_get_song_length(const DeckPlayer* player) {
    if (!player) return 0;

    switch (player->type) {
        case DECK_PLAYER_MOD:
            return mod_player_get_song_length(player->mod_player);
        case DECK_PLAYER_MED:
            return med_player_get_song_length(player->med_player);
        case DECK_PLAYER_AHX:
            // AHX doesn't expose song length API
            return 0;
        default:
            return 0;
    }
}

uint8_t deck_player_get_num_channels(const DeckPlayer* player) {
    if (!player) return 0;

    switch (player->type) {
        case DECK_PLAYER_MOD:
            return 4;  // MOD is always 4 channels
        case DECK_PLAYER_MED:
            return med_player_get_num_channels(player->med_player);
        case DECK_PLAYER_AHX:
            return 4;  // AHX is always 4 channels
        default:
            return 0;
    }
}

uint16_t deck_player_get_bpm(const DeckPlayer* player) {
    if (!player) return 125;

    switch (player->type) {
        case DECK_PLAYER_MOD:
            return 125;  // MOD doesn't expose BPM API (uses CIA tempo)
        case DECK_PLAYER_MED:
            return med_player_get_bpm(player->med_player);
        case DECK_PLAYER_AHX:
            return 125;  // AHX doesn't expose BPM (uses CIA tempo)
        default:
            return 125;
    }
}

void deck_player_set_bpm(DeckPlayer* player, uint16_t bpm) {
    if (!player) return;

    switch (player->type) {
        case DECK_PLAYER_MOD:
            // MOD doesn't support BPM API
            (void)bpm;
            break;
        case DECK_PLAYER_MED:
            med_player_set_bpm(player->med_player, bpm);
            break;
        case DECK_PLAYER_AHX:
            // AHX doesn't support BPM changes
            break;
        default:
            break;
    }
}

void deck_player_set_loop_range(DeckPlayer* player, uint16_t start_order, uint16_t end_order) {
    if (!player) return;

    switch (player->type) {
        case DECK_PLAYER_MOD:
            mod_player_set_loop_range(player->mod_player, start_order, end_order);
            break;
        case DECK_PLAYER_MED:
            med_player_set_loop_range(player->med_player, start_order, end_order);
            break;
        case DECK_PLAYER_AHX:
            // AHX doesn't support loop range
            break;
        default:
            break;
    }
}

void deck_player_set_channel_mute(DeckPlayer* player, uint8_t channel, bool muted) {
    if (!player || channel >= 4) return;

    player->channel_muted[channel] = muted;

    switch (player->type) {
        case DECK_PLAYER_MOD:
            mod_player_set_channel_mute(player->mod_player, channel, muted);
            break;
        case DECK_PLAYER_MED:
            med_player_set_channel_mute(player->med_player, channel, muted);
            break;
        case DECK_PLAYER_AHX:
            ahx_player_set_channel_mute(player->ahx_player, channel, muted);
            break;
        default:
            break;
    }
}

bool deck_player_get_channel_mute(const DeckPlayer* player, uint8_t channel) {
    if (!player || channel >= 4) return false;
    return player->channel_muted[channel];
}

void deck_player_process_channels(DeckPlayer* player,
                                   float* left,
                                   float* right,
                                   float* channel_outputs[4],
                                   size_t num_samples,
                                   int sample_rate) {
    if (!player || !left || !right) return;

    // Clear outputs
    memset(left, 0, num_samples * sizeof(float));
    memset(right, 0, num_samples * sizeof(float));

    switch (player->type) {
        case DECK_PLAYER_MOD:
            mod_player_process_channels(player->mod_player, left, right,
                                       channel_outputs, num_samples, sample_rate);
            break;
        case DECK_PLAYER_MED:
            med_player_process_channels(player->med_player, left, right,
                                       channel_outputs, 4, num_samples, sample_rate);
            break;
        case DECK_PLAYER_AHX:
            ahx_player_process_channels(player->ahx_player, left, right,
                                       channel_outputs, num_samples, sample_rate);
            break;
        default:
            break;
    }
}

void deck_player_process(DeckPlayer* player, float* left, float* right, size_t num_samples, int sample_rate) {
    deck_player_process_channels(player, left, right, NULL, num_samples, sample_rate);
}

// Internal position callbacks that forward to user callback
static void mod_position_callback(uint8_t order, uint8_t pattern, uint16_t row, void* user_data) {
    DeckPlayer* player = (DeckPlayer*)user_data;
    if (player && player->position_callback) {
        player->position_callback(order, pattern, row, player->position_callback_userdata);
    }
}

static void med_position_callback(uint8_t order, uint8_t pattern, uint16_t row, void* user_data) {
    DeckPlayer* player = (DeckPlayer*)user_data;
    if (player && player->position_callback) {
        player->position_callback(order, pattern, row, player->position_callback_userdata);
    }
}

static void ahx_position_callback(uint8_t subsong, uint16_t position, uint16_t row, void* user_data) {
    DeckPlayer* player = (DeckPlayer*)user_data;
    if (player && player->position_callback) {
        player->position_callback(subsong, position, row, player->position_callback_userdata);
    }
}

PatternSequencer* deck_player_get_sequencer(DeckPlayer* player) {
    if (!player) return NULL;

    switch (player->type) {
        case DECK_PLAYER_MOD:
            return mod_player_get_sequencer(player->mod_player);
        case DECK_PLAYER_MED:
            return med_player_get_sequencer(player->med_player);
        case DECK_PLAYER_AHX:
            return ahx_player_get_sequencer(player->ahx_player);
        default:
            return NULL;
    }
}
