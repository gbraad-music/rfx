/*
 * Tracker Mixer - Shared stereo mixing for tracker-based players
 *
 * Provides common stereo mixing functionality for MOD, MMD, and AHX players.
 *
 * All three players use a frame-based approach:
 * - Effects can change parameters every sample (vibrato, tremolo)
 * - Tick boundaries require frequent effect processing
 * - Float-based processing for clarity and consistency
 * - Per-channel outputs for VCV Rack integration
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
