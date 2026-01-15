#ifndef MMD_PLAYER_H
#define MMD_PLAYER_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// Forward declaration
typedef struct MedPlayer MedPlayer;

// Position callback - called when playback position changes
typedef void (*MedPositionCallback)(uint8_t order, uint8_t pattern, uint16_t row, void* user_data);

/**
 * Create a new MED player instance
 * @return Pointer to MedPlayer, or NULL on failure
 */
MedPlayer* med_player_create(void);

/**
 * Destroy a MED player instance
 * @param player Player to destroy
 */
void med_player_destroy(MedPlayer* player);

/**
 * Check if data is a valid MMD file
 * @param data Pointer to file data
 * @param size Size of file in bytes
 * @return true if data appears to be a valid MMD2/MMD3 file
 */
bool med_player_detect(const uint8_t* data, size_t size);

/**
 * Load an MMD2 file from memory
 * @param player Player instance
 * @param data File data
 * @param size File size in bytes
 * @return true on success, false on failure
 */
bool med_player_load(MedPlayer* player, const uint8_t* data, size_t size);

/**
 * Start playback
 * @param player Player instance
 */
void med_player_start(MedPlayer* player);

/**
 * Stop playback
 * @param player Player instance
 */
void med_player_stop(MedPlayer* player);

/**
 * Check if player is currently playing
 * @param player Player instance
 * @return true if playing, false otherwise
 */
bool med_player_is_playing(const MedPlayer* player);

/**
 * Process audio (render samples)
 * @param player Player instance
 * @param left_out Left channel output buffer
 * @param right_out Right channel output buffer
 * @param frames Number of frames to render
 * @param sample_rate Output sample rate
 */
void med_player_process(MedPlayer* player, float* left_out, float* right_out,
                        size_t frames, float sample_rate);

/**
 * Process audio with per-channel outputs
 * @param player Player instance
 * @param left_out Left channel output buffer (mixed)
 * @param right_out Right channel output buffer (mixed)
 * @param channel_outputs Array of pointers to mono channel buffers (can be NULL if not needed)
 * @param num_channel_outputs Size of channel_outputs array (use med_player_get_num_channels())
 * @param frames Number of frames to render
 * @param sample_rate Output sample rate
 *
 * Use case: VCV Rack modules can expose each channel as a separate output for
 * individual processing, effects, or visualization
 *
 * Note: MMD can have 4-64 channels. Use med_player_get_num_channels() to determine
 * how many channel buffers to allocate. Buffers beyond the actual channel count
 * will be filled with silence.
 */
void med_player_process_channels(MedPlayer* player,
                                 float* left_out,
                                 float* right_out,
                                 float** channel_outputs,
                                 uint8_t num_channel_outputs,
                                 size_t frames,
                                 float sample_rate);

/**
 * Get current playback position
 * @param player Player instance
 * @param out_pattern Current pattern index
 * @param out_row Current row within pattern
 */
void med_player_get_position(const MedPlayer* player, uint8_t* out_pattern, uint16_t* out_row);

/**
 * Set playback position
 * @param player Player instance
 * @param pattern Pattern index
 * @param row Row within pattern
 */
void med_player_set_position(MedPlayer* player, uint8_t pattern, uint16_t row);

/**
 * Get song length (number of patterns in play sequence)
 * @param player Player instance
 * @return Number of patterns
 */
uint8_t med_player_get_song_length(const MedPlayer* player);

/**
 * Set position callback
 * @param player Player instance
 * @param callback Callback function (NULL to disable)
 * @param user_data User data passed to callback
 */
void med_player_set_position_callback(MedPlayer* player, MedPositionCallback callback, void* user_data);

/**
 * Set channel mute state
 * @param player Player instance
 * @param channel Channel index (0-63)
 * @param muted true to mute, false to unmute
 */
void med_player_set_channel_mute(MedPlayer* player, uint8_t channel, bool muted);

/**
 * Get channel mute state
 * @param player Player instance
 * @param channel Channel index (0-63)
 * @return true if muted, false if not
 */
bool med_player_get_channel_mute(const MedPlayer* player, uint8_t channel);

/**
 * Set channel volume
 * @param player Player instance
 * @param channel Channel index (0-63)
 * @param volume Volume (0.0 to 1.0)
 */
void med_player_set_channel_volume(MedPlayer* player, uint8_t channel, float volume);

/**
 * Get channel volume
 * @param player Player instance
 * @param channel Channel index (0-63)
 * @return Volume (0.0 to 1.0)
 */
float med_player_get_channel_volume(const MedPlayer* player, uint8_t channel);

/**
 * Set BPM tempo
 * @param player Player instance
 * @param bpm Beats per minute
 */
void med_player_set_bpm(MedPlayer* player, uint16_t bpm);

/**
 * Get BPM tempo
 * @param player Player instance
 * @return Beats per minute
 */
uint16_t med_player_get_bpm(const MedPlayer* player);

/**
 * Set loop range (pattern order indices)
 * @param player Player instance
 * @param start_order First order to play (0-song_length-1)
 * @param end_order Last order to play (0-song_length-1)
 * If start_order == end_order, only that order will loop
 */
void med_player_set_loop_range(MedPlayer* player, uint16_t start_order, uint16_t end_order);

/**
 * Enable or disable looping
 * @param player Player instance
 * @param disable If true, playback stops at end instead of looping
 */
void med_player_set_disable_looping(MedPlayer* player, bool disable);

/**
 * Get number of channels/tracks in loaded song
 * @param player Player instance
 * @return Number of channels (4-64)
 */
uint8_t med_player_get_num_channels(const MedPlayer* player);

/**
 * Get underlying PatternSequencer (for advanced control with RegrooveController)
 * WARNING: Do not destroy the returned sequencer - it's owned by the player
 * @param player Player instance
 * @return Pattern sequencer instance or NULL
 */
struct PatternSequencer* med_player_get_sequencer(MedPlayer* player);

#ifdef __cplusplus
}
#endif

#endif // MED_PLAYER_H
