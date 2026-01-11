/*
 * Generic Tracker Voice
 *
 * Fixed-point wavetable playback used in Amiga-style trackers.
 * Uses 16.16 fixed-point arithmetic for sub-sample precision.
 *
 * Based on patterns from ProTracker, AHX/HVL, OctaMED, etc.
 */

#ifndef TRACKER_VOICE_H
#define TRACKER_VOICE_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    // Fixed-point playback state (16.16)
    uint32_t sample_pos;    // Current position in waveform
    uint32_t delta;         // Frequency/pitch (added to sample_pos each sample)

    // Waveform data
    const void* waveform;   // Pointer to waveform data (int8_t* or int16_t*)
    uint32_t length;        // Length in samples (in fixed-point: length << 16)
    uint8_t bit_depth;      // Bit depth: 8 or 16

    // Loop points (16.16 fixed-point)
    uint32_t loop_start;    // Loop start position
    uint32_t loop_end;      // Loop end position
    bool loop_enabled;      // If true: loop, if false: play once and stop

    // Volume
    int32_t volume;         // 0-64 (tracker-style volume)

    // Panning (0-255, 128 = center)
    int32_t pan_left;       // Left channel multiplier
    int32_t pan_right;      // Right channel multiplier
} TrackerVoice;

/**
 * Initialize voice
 */
void tracker_voice_init(TrackerVoice* voice);

/**
 * Set 8-bit waveform data
 * @param waveform Pointer to int8_t waveform data
 * @param length Length in samples
 */
void tracker_voice_set_waveform(TrackerVoice* voice,
                                const int8_t* waveform,
                                uint32_t length);

/**
 * Set 16-bit waveform data
 * @param waveform Pointer to int16_t waveform data
 * @param length Length in samples
 */
void tracker_voice_set_waveform_16bit(TrackerVoice* voice,
                                      const int16_t* waveform,
                                      uint32_t length);

/**
 * Set frequency using period (Amiga-style)
 * @param period Amiga period value
 * @param clock_rate Paula clock rate (e.g. 3546895 for PAL)
 * @param sample_rate Output sample rate
 */
void tracker_voice_set_period(TrackerVoice* voice,
                              uint32_t period,
                              uint32_t clock_rate,
                              uint32_t sample_rate);

/**
 * Set frequency using delta directly (16.16 fixed-point)
 * @param delta Fixed-point delta value
 */
void tracker_voice_set_delta(TrackerVoice* voice, uint32_t delta);

/**
 * Set volume (0-64)
 */
void tracker_voice_set_volume(TrackerVoice* voice, int32_t volume);

/**
 * Set panning using pre-calculated multipliers
 * @param pan_left Left multiplier (0-255)
 * @param pan_right Right multiplier (0-255)
 */
void tracker_voice_set_panning(TrackerVoice* voice,
                               int32_t pan_left,
                               int32_t pan_right);

/**
 * Set loop points
 * @param loop_start Loop start position in bytes
 * @param loop_length Loop length in bytes (if <= 1, disables looping)
 *
 * Note: Internally converts bytes to samples based on bit_depth
 */
void tracker_voice_set_loop(TrackerVoice* voice,
                             uint32_t loop_start,
                             uint32_t loop_length);

/**
 * Reset playback position
 */
void tracker_voice_reset_position(TrackerVoice* voice);

/**
 * Get next sample (no interpolation)
 * @return Sample value (8-bit: -128 to 127, 16-bit: -32768 to 32767)
 */
int32_t tracker_voice_get_sample(TrackerVoice* voice);

/**
 * Get next sample with volume applied
 * @return Sample value scaled by volume
 */
int32_t tracker_voice_get_sample_scaled(TrackerVoice* voice);

/**
 * Get stereo sample pair (volume + panning applied)
 * @param out_left Output for left channel
 * @param out_right Output for right channel
 */
void tracker_voice_get_stereo_sample(TrackerVoice* voice,
                                     int32_t* out_left,
                                     int32_t* out_right);

#ifdef __cplusplus
}
#endif

#endif // TRACKER_VOICE_H
