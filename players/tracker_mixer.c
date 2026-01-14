/*
 * Tracker Mixer Implementation
 */

#include "tracker_mixer.h"

void tracker_mixer_mix_stereo(const TrackerMixerChannel* channels,
                               uint32_t num_channels,
                               float* left_out,
                               float* right_out,
                               float scaling) {
    float left_sum = 0.0f;
    float right_sum = 0.0f;

    // Mix all enabled channels
    for (uint32_t i = 0; i < num_channels; i++) {
        if (!channels[i].enabled) continue;

        float sample = channels[i].sample;
        float pan = channels[i].panning;

        // Convert panning to gains
        float left_gain, right_gain;
        tracker_mixer_pan_to_gains(pan, &left_gain, &right_gain);

        // Mix into stereo
        left_sum += sample * left_gain;
        right_sum += sample * right_gain;
    }

    // Apply final scaling and output
    *left_out = left_sum * scaling;
    *right_out = right_sum * scaling;
}

void tracker_mixer_pan_to_gains(float pan, float* left_gain, float* right_gain) {
    // Clamp panning to valid range
    if (pan < -1.0f) pan = -1.0f;
    if (pan > 1.0f) pan = 1.0f;

    // Convert -1..1 to 0..1 range
    float pan_01 = pan * 0.5f + 0.5f;

    // Calculate gains
    // pan = -1.0 (left):  left_gain = 1.0, right_gain = 0.0
    // pan =  0.0 (center): left_gain = 1.0, right_gain = 1.0
    // pan =  1.0 (right): left_gain = 0.0, right_gain = 1.0
    *left_gain = 1.0f - pan_01;
    *right_gain = pan_01;
}

float tracker_mixer_mmd_pan_to_normalized(int8_t mmd_pan) {
    // MMD uses -16 (left) to +16 (right)
    // Clamp to valid range
    if (mmd_pan < -16) mmd_pan = -16;
    if (mmd_pan > 16) mmd_pan = 16;

    // Convert to -1.0 to 1.0
    return (float)mmd_pan / 16.0f;
}
