#ifndef DECK_PLAYER_H
#define DECK_PLAYER_H

/**
 * Unified Deck Player - Handles MOD/MED/AHX files with automatic format detection
 *
 * This provides a simple unified interface for playing tracker modules,
 * handling format detection and routing to the appropriate player automatically.
 */

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// Opaque deck player structure
typedef struct DeckPlayer DeckPlayer;

// Player type (read-only, for informational purposes)
typedef enum {
    DECK_PLAYER_NONE = 0,
    DECK_PLAYER_MOD,
    DECK_PLAYER_MED,
    DECK_PLAYER_AHX
} DeckPlayerType;

// Position callback - unified signature
// order/subsong, pattern/position, row
typedef void (*DeckPlayerPositionCallback)(uint8_t order, uint16_t pattern, uint16_t row, void* user_data);

// Create a new deck player instance
DeckPlayer* deck_player_create(void);

// Destroy deck player instance
void deck_player_destroy(DeckPlayer* player);

// Load a file from memory (auto-detects format)
// Returns true on success, false on failure
bool deck_player_load(DeckPlayer* player, const uint8_t* data, size_t size);

// Get current player type (after loading)
DeckPlayerType deck_player_get_type(const DeckPlayer* player);

// Get type name as string
const char* deck_player_get_type_name(const DeckPlayer* player);

// Get song title (returns NULL if no file loaded)
const char* deck_player_get_title(const DeckPlayer* player);

// Playback control
void deck_player_start(DeckPlayer* player);
void deck_player_stop(DeckPlayer* player);
bool deck_player_is_playing(const DeckPlayer* player);

// Position callback
void deck_player_set_position_callback(DeckPlayer* player, DeckPlayerPositionCallback callback, void* user_data);

// Get current playback position
void deck_player_get_position(const DeckPlayer* player, uint8_t* order, uint16_t* pattern, uint16_t* row);

// Set playback position
void deck_player_set_position(DeckPlayer* player, uint8_t order, uint16_t row);

// Get song info
uint8_t deck_player_get_song_length(const DeckPlayer* player);
uint8_t deck_player_get_num_channels(const DeckPlayer* player);
uint16_t deck_player_get_bpm(const DeckPlayer* player);
void deck_player_set_bpm(DeckPlayer* player, uint16_t bpm);

// Loop control
void deck_player_set_loop_range(DeckPlayer* player, uint16_t start_order, uint16_t end_order);

// Channel muting (channels 0-3, supports up to 4 channels)
void deck_player_set_channel_mute(DeckPlayer* player, uint8_t channel, bool muted);
bool deck_player_get_channel_mute(const DeckPlayer* player, uint8_t channel);

// Render audio samples with per-channel outputs
// - left/right: stereo mix output (Amiga panning [L R R L])
// - channel_outputs: array of 4 mono channel buffers (can be NULL)
// - num_samples: number of samples to render
// - sample_rate: output sample rate (typically 48000)
void deck_player_process_channels(DeckPlayer* player,
                                   float* left,
                                   float* right,
                                   float* channel_outputs[4],
                                   size_t num_samples,
                                   int sample_rate);

// Render audio samples (stereo mix only)
void deck_player_process(DeckPlayer* player, float* left, float* right, size_t num_samples, int sample_rate);

#ifdef __cplusplus
}
#endif

#endif // DECK_PLAYER_H
