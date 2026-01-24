/*
 * Generic Tracker Voice Implementation
 */

#include "tracker_voice.h"
#include <string.h>
#include <stdio.h>

// Convert period to frequency (16.16 fixed-point delta)
#define Period2Delta(period, clock_rate, sample_rate) \
    ((uint32_t)(((uint64_t)(clock_rate) * 65536ULL) / ((period) * (sample_rate))))

void tracker_voice_init(TrackerVoice* voice) {
    memset(voice, 0, sizeof(TrackerVoice));
    voice->delta = 1;  // Avoid division by zero
    voice->volume = 64;
    voice->pan_left = 255;
    voice->pan_right = 255;
    voice->bit_depth = 8;  // Default to 8-bit
    voice->loop_enabled = true;  // Default: loop enabled
}

void tracker_voice_set_waveform(TrackerVoice* voice,
                                const int8_t* waveform,
                                uint32_t length) {
    voice->waveform = (const void*)waveform;
    voice->length = ((uint64_t)length) << 16;  // Convert to fixed-point (uint64_t for long samples)
    voice->bit_depth = 8;
    // Default loop: full sample
    voice->loop_start = 0;
    voice->loop_end = voice->length;
    voice->loop_enabled = true;
}

void tracker_voice_set_waveform_16bit(TrackerVoice* voice,
                                      const int16_t* waveform,
                                      uint32_t length) {
    voice->waveform = (const void*)waveform;
    voice->length = ((uint64_t)length) << 16;  // Convert to fixed-point (uint64_t for long samples)
    voice->bit_depth = 16;
    // Default loop: full sample
    voice->loop_start = 0;
    voice->loop_end = voice->length;
    voice->loop_enabled = true;
}

void tracker_voice_set_period(TrackerVoice* voice,
                              uint32_t period,
                              uint32_t clock_rate,
                              uint32_t sample_rate) {
    if (period == 0) {
        voice->delta = 0;
        return;
    }

    uint32_t new_delta = Period2Delta(period, clock_rate, sample_rate);

    #ifdef DEBUG_PERIOD
    if (voice->delta != new_delta) {
        fprintf(stderr, "    tracker_voice_set_period: period=%d -> delta=%u (was %u)\n",
                period, new_delta, voice->delta);
    }
    #endif

    voice->delta = new_delta;

    // Delta is playback SPEED (16.16 fixed point), not position
    // It should never be clamped or wrapped based on waveform length
    // Only the playback position wraps during get_sample()

    if (voice->delta == 0) {
        voice->delta = 1;
    }
}

void tracker_voice_set_delta(TrackerVoice* voice, uint32_t delta) {
    voice->delta = delta;
    if (voice->delta == 0) {
        voice->delta = 1;
    }
}

void tracker_voice_set_volume(TrackerVoice* voice, int32_t volume) {
    if (volume < 0) volume = 0;
    if (volume > 64) volume = 64;
    voice->volume = volume;
}

void tracker_voice_set_panning(TrackerVoice* voice,
                               int32_t pan_left,
                               int32_t pan_right) {
    voice->pan_left = pan_left;
    voice->pan_right = pan_right;
}

void tracker_voice_set_loop(TrackerVoice* voice,
                             uint32_t loop_start,
                             uint32_t loop_length,
                             uint32_t one_shot_threshold_bytes) {
    // Convert bytes to samples based on bit depth
    uint32_t loop_start_samples = loop_start;
    uint32_t loop_length_samples = loop_length;
    uint32_t threshold_samples = one_shot_threshold_bytes;

    if (voice->bit_depth == 16) {
        loop_start_samples /= 2;
        loop_length_samples /= 2;
        threshold_samples /= 2;
    }

    bool is_oneshot = (loop_length_samples <= threshold_samples);

    if (is_oneshot) {
        // "One-shot" sample - play full length from 0 to end, then stop
        voice->loop_start = 0;
        voice->loop_end = voice->length;  // Play to full sample length
        voice->loop_enabled = false;  // Disable looping - sample plays once and stops
    } else {
        // Normal looping sample - use specified loop points (uint64_t for long samples)
        voice->loop_start = ((uint64_t)loop_start_samples) << 16;
        voice->loop_end = ((uint64_t)(loop_start_samples + loop_length_samples)) << 16;
        voice->loop_enabled = true;
    }
}

void tracker_voice_reset_position(TrackerVoice* voice) {
    voice->sample_pos = 0;
}

void tracker_voice_set_position(TrackerVoice* voice, uint32_t byte_offset) {
    // Convert bytes to samples based on bit depth
    uint32_t sample_offset = byte_offset;
    if (voice->bit_depth == 16) {
        sample_offset /= 2;
    }
    // Convert to 16.16 fixed-point (now uint64_t to support long samples)
    voice->sample_pos = ((uint64_t)sample_offset) << 16;
}

int32_t tracker_voice_get_sample(TrackerVoice* voice) {
    if (!voice->waveform || voice->length == 0) {
        return 0;
    }

    // Get sample at current integer position (16.16 fixed-point)
    uint64_t pos = voice->sample_pos >> 16;

    // Safety clamp to prevent buffer overrun
    uint64_t max_pos = (voice->length >> 16);
    if (pos >= max_pos) {
        // Past end of sample - return silence
        return 0;
    }

    int32_t sample;

    if (voice->bit_depth == 16) {
        const int16_t* waveform16 = (const int16_t*)voice->waveform;
        sample = waveform16[pos];
    } else {
        const int8_t* waveform8 = (const int8_t*)voice->waveform;
        sample = waveform8[pos];
    }

    // Advance position for NEXT sample
    voice->sample_pos += voice->delta;

    // Check if we need to wrap for next call
    if (voice->sample_pos >= voice->loop_end) {
        if (voice->loop_enabled) {
            // Wrap to loop start
            uint64_t loop_len = voice->loop_end - voice->loop_start;
            if (loop_len > 0) {
                // Handle case where delta causes multiple wraps
                while (voice->sample_pos >= voice->loop_end) {
                    voice->sample_pos -= loop_len;
                }
                // Ensure we're in the loop region
                if (voice->sample_pos < voice->loop_start) {
                    voice->sample_pos += loop_len;
                }
            } else {
                voice->sample_pos = voice->loop_start;
            }
        }
        // For one-shot samples (loop_enabled=false), position stays past the end
        // and next call will return silence
    }

    return sample;
}

int32_t tracker_voice_get_sample_scaled(TrackerVoice* voice) {
    int32_t sample = tracker_voice_get_sample(voice);
    return sample * voice->volume;
}

void tracker_voice_get_stereo_sample(TrackerVoice* voice,
                                     int32_t* out_left,
                                     int32_t* out_right) {
    // Use the main get_sample function which handles looping correctly
    int32_t sample = tracker_voice_get_sample(voice);

    // Apply volume
    int32_t scaled = sample * voice->volume;

    // Apply panning
    *out_left = (scaled * voice->pan_left) >> 7;
    *out_right = (scaled * voice->pan_right) >> 7;
}
