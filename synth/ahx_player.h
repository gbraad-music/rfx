#ifndef AHX_PLAYER_H
#define AHX_PLAYER_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// Opaque player structure
typedef struct AhxPlayer AhxPlayer;

// Position callback - called when playback position changes
typedef void (*AhxPositionCallback)(uint8_t subsong, uint16_t position, uint16_t row, void* user_data);

// Create a new AHX player instance
AhxPlayer* ahx_player_create(void);

// Destroy player instance
void ahx_player_destroy(AhxPlayer* player);

// Load AHX song from memory buffer
// Returns true on success, false on failure
bool ahx_player_load(AhxPlayer* player, const uint8_t* data, size_t size);

// Set subsong to play (0-based index)
void ahx_player_set_subsong(AhxPlayer* player, uint8_t subsong);

// Get current subsong
uint8_t ahx_player_get_current_subsong(const AhxPlayer* player);

// Get number of subsongs
uint8_t ahx_player_get_num_subsongs(const AhxPlayer* player);

// Get song title (returns NULL if no song loaded)
const char* ahx_player_get_title(const AhxPlayer* player);

// Start playback
void ahx_player_start(AhxPlayer* player);

// Stop playback
void ahx_player_stop(AhxPlayer* player);

// Check if player is currently playing
bool ahx_player_is_playing(const AhxPlayer* player);

// Set position callback
void ahx_player_set_position_callback(AhxPlayer* player, AhxPositionCallback callback, void* user_data);

// Get current playback position
void ahx_player_get_position(const AhxPlayer* player, uint16_t* position, uint16_t* row);

// Render audio samples
// Outputs stereo float samples in range [-1.0, 1.0]
// left and right arrays must have space for 'num_samples' floats
void ahx_player_process(AhxPlayer* player, float* left, float* right, size_t num_samples, int sample_rate);

/**
 * Process with per-channel outputs
 * @param player Player instance
 * @param left Left channel output buffer (mixed)
 * @param right Right channel output buffer (mixed)
 * @param channel_outputs Array of 4 mono channel buffers (can be NULL if not needed)
 * @param num_samples Number of samples to render
 * @param sample_rate Output sample rate (typically 48000)
 *
 * Use case: VCV Rack modules can expose each channel as a separate output for
 * individual processing, effects, or visualization
 */
void ahx_player_process_channels(AhxPlayer* player,
                                   float* left,
                                   float* right,
                                   float* channel_outputs[4],
                                   size_t num_samples,
                                   int sample_rate);

// Set channel mute state (channel 0-3)
void ahx_player_set_channel_mute(AhxPlayer* player, uint8_t channel, bool muted);

// Get channel mute state
bool ahx_player_get_channel_mute(const AhxPlayer* player, uint8_t channel);

// Set master volume boost (default 1.0)
void ahx_player_set_boost(AhxPlayer* player, float boost);

// Enable/disable oversampling (interpolation)
void ahx_player_set_oversampling(AhxPlayer* player, bool enabled);

// Disable looping (for rendering to file)
void ahx_player_set_disable_looping(AhxPlayer* player, bool disable);

#ifdef __cplusplus
}
#endif

#endif // AHX_PLAYER_H
