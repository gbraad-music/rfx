/*
 * Tracker Mixer - Shared stereo mixing for tracker-based players
 *
 * Provides common stereo mixing functionality for MOD and MMD players.
 *
 * NOTE: AHX player does NOT use this shared mixer because it uses a different
 * optimization strategy:
 * - Batch processing: Calculates how many samples can be rendered before any
 *   voice wraps, then processes that batch in a tight inner loop
 * - Fixed-point integer math: Uses >>7, >>8 shifts instead of float division
 * - Constant parameters: AHX effects don't change per-sample like MOD vibrato
 * - Pre-computed sin tables: Uses lookup tables for panning calculations
 *
 * This shared mixer is designed for frame-based players (MOD/MMD) where:
 * - Effects can change parameters every sample (vibrato, tremolo)
 * - Tick boundaries require frequent effect processing
 * - Float-based processing is acceptable for clarity
 */

#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Channel data for mixing
 */
typedef struct {
    float sample;           // Mono sample value (already volume-scaled)
    float panning;          // Panning: -1.0 = hard left, 0.0 = center, 1.0 = hard right
    int enabled;            // 1 = mix this channel, 0 = skip
} TrackerMixerChannel;

/**
 * Mix mono channels into stereo output
 *
 * @param channels Array of channel data
 * @param num_channels Number of channels to mix
 * @param left_out Left channel output
 * @param right_out Right channel output
 * @param scaling Final output scaling factor (e.g., 0.5f for Amiga-style headroom)
 */
void tracker_mixer_mix_stereo(const TrackerMixerChannel* channels,
                               uint32_t num_channels,
                               float* left_out,
                               float* right_out,
                               float scaling);

/**
 * Convert MOD-style panning (-1.0 to 1.0) to gain values
 *
 * @param pan Panning value (-1.0 = left, 0.0 = center, 1.0 = right)
 * @param left_gain Output: left channel gain (0.0 to 1.0)
 * @param right_gain Output: right channel gain (0.0 to 1.0)
 */
void tracker_mixer_pan_to_gains(float pan, float* left_gain, float* right_gain);

/**
 * Convert MMD-style panning (-16 to +16) to normalized panning (-1.0 to 1.0)
 *
 * @param mmd_pan MMD panning value (-16 to +16)
 * @return Normalized panning (-1.0 to 1.0)
 */
float tracker_mixer_mmd_pan_to_normalized(int8_t mmd_pan);

#ifdef __cplusplus
}
#endif
