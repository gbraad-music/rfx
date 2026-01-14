#ifndef SID_PLAYER_H
#define SID_PLAYER_H

/**
 * SID Player - Commodore 64 SID Music Player
 *
 * Features:
 * - PSID/RSID file playback
 * - Multiple subsong support
 * - Minimal 6502 emulation for player code
 * - 3-voice SID chip emulation
 * - PAL/NTSC timing support
 * - Per-voice mute control
 * - Per-voice output for external processing
 */

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Opaque player structure */
typedef struct SidPlayer SidPlayer;

/* Position callback - called when playback position changes */
typedef void (*SidPositionCallback)(uint8_t subsong, uint32_t time_ms, void* user_data);

/* ============================================================================
 * Lifecycle
 * ============================================================================ */

/**
 * Create a new SID player instance
 * @return New player instance or NULL on failure
 */
SidPlayer* sid_player_create(void);

/**
 * Destroy player instance and free resources
 */
void sid_player_destroy(SidPlayer* player);

/* ============================================================================
 * File Loading
 * ============================================================================ */

/**
 * Check if data is a valid SID file (PSID/RSID)
 * @param data File data buffer
 * @param size Size of data in bytes
 * @return true if data appears to be a valid SID file
 */
bool sid_player_detect(const uint8_t* data, size_t size);

/**
 * Load SID file from memory buffer
 * @param player Player instance
 * @param data File data buffer
 * @param size Size of data in bytes
 * @return true on success, false on failure
 */
bool sid_player_load(SidPlayer* player, const uint8_t* data, size_t size);

/* ============================================================================
 * Playback Control
 * ============================================================================ */

/**
 * Start playback
 */
void sid_player_start(SidPlayer* player);

/**
 * Stop playback
 */
void sid_player_stop(SidPlayer* player);

/**
 * Check if player is currently playing
 * @return true if playing, false otherwise
 */
bool sid_player_is_playing(const SidPlayer* player);

/* ============================================================================
 * Song Information
 * ============================================================================ */

/**
 * Set subsong to play (0-based index)
 * @param subsong Subsong number (0 to num_subsongs-1)
 */
void sid_player_set_subsong(SidPlayer* player, uint8_t subsong);

/**
 * Get current subsong
 * @return Current subsong number
 */
uint8_t sid_player_get_current_subsong(const SidPlayer* player);

/**
 * Get number of subsongs
 * @return Number of subsongs in file (1 if not multi-song)
 */
uint8_t sid_player_get_num_subsongs(const SidPlayer* player);

/**
 * Get song title (returns NULL if no song loaded)
 * @return Song title string or NULL
 */
const char* sid_player_get_title(const SidPlayer* player);

/**
 * Get song author (returns NULL if no song loaded)
 * @return Author name string or NULL
 */
const char* sid_player_get_author(const SidPlayer* player);

/**
 * Get song copyright/release info (returns NULL if no song loaded)
 * @return Copyright string or NULL
 */
const char* sid_player_get_copyright(const SidPlayer* player);

/* ============================================================================
 * Playback Position
 * ============================================================================ */

/**
 * Set position callback
 * @param callback Function to call on position changes
 * @param user_data User data passed to callback
 */
void sid_player_set_position_callback(SidPlayer* player,
                                       SidPositionCallback callback,
                                       void* user_data);

/**
 * Get current playback time in milliseconds
 * @return Playback time in milliseconds
 */
uint32_t sid_player_get_time_ms(const SidPlayer* player);

/* ============================================================================
 * Audio Rendering
 * ============================================================================ */

/**
 * Render stereo audio samples
 * Outputs stereo float samples in range [-1.0, 1.0]
 * @param player Player instance
 * @param left Left channel output buffer
 * @param right Right channel output buffer
 * @param num_samples Number of samples to render
 * @param sample_rate Output sample rate (typically 48000)
 */
void sid_player_process(SidPlayer* player,
                        float* left,
                        float* right,
                        size_t num_samples,
                        int sample_rate);

/**
 * Process with per-voice outputs
 * @param player Player instance
 * @param left Left channel output buffer (mixed)
 * @param right Right channel output buffer (mixed)
 * @param voice_outputs Array of 3 mono voice buffers (can be NULL if not needed)
 * @param num_samples Number of samples to render
 * @param sample_rate Output sample rate (typically 48000)
 *
 * Use case: External processing of individual voices for effects or visualization
 */
void sid_player_process_voices(SidPlayer* player,
                               float* left,
                               float* right,
                               float* voice_outputs[3],
                               size_t num_samples,
                               int sample_rate);

/* ============================================================================
 * Mixer Controls
 * ============================================================================ */

/**
 * Set voice mute state (voice 0-2)
 * @param voice Voice number (0-2)
 * @param muted true to mute, false to unmute
 */
void sid_player_set_voice_mute(SidPlayer* player, uint8_t voice, bool muted);

/**
 * Get voice mute state
 * @param voice Voice number (0-2)
 * @return true if muted, false otherwise
 */
bool sid_player_get_voice_mute(const SidPlayer* player, uint8_t voice);

/**
 * Set master volume boost (default 1.0)
 * @param boost Volume multiplier (1.0 = normal, 2.0 = double, etc.)
 */
void sid_player_set_boost(SidPlayer* player, float boost);

/**
 * Disable looping (for rendering to file)
 * @param disable true to disable looping, false to enable
 */
void sid_player_set_disable_looping(SidPlayer* player, bool disable);

/* ============================================================================
 * Debug/Analysis
 * ============================================================================ */

/**
 * Enable tracker-style debug output
 * Shows register writes, notes, waveforms in real-time
 * @param enabled true to enable debug output, false to disable
 */
void sid_player_set_debug_output(SidPlayer* player, bool enabled);

/**
 * Print current SID register state snapshot
 * Useful for capturing exact state at a specific moment
 */
void sid_player_print_state(SidPlayer* player);

#ifdef __cplusplus
}
#endif

#endif /* SID_PLAYER_H */
