/*
 * Generic Tracker Voice Implementation
 */

#include "tracker_voice.h"
#include <string.h>

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
    voice->length = length << 16;  // Convert to fixed-point
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
    voice->length = length << 16;  // Convert to fixed-point
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

    voice->delta = Period2Delta(period, clock_rate, sample_rate);

    // Wraparound handling (for looping waveforms)
    if (voice->delta > voice->length) {
        voice->delta -= voice->length;
    }

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
                             uint32_t loop_length) {
    // Convert bytes to samples based on bit depth
    uint32_t loop_start_samples = loop_start;
    uint32_t loop_length_samples = loop_length;

    if (voice->bit_depth == 16) {
        loop_start_samples /= 2;
        loop_length_samples /= 2;
    }

    voice->loop_start = loop_start_samples << 16;  // Convert to fixed-point

    if (loop_length_samples <= 1) {
        // One-shot sample (no loop)
        voice->loop_enabled = false;
        voice->loop_end = voice->length;
    } else {
        // Looping sample
        voice->loop_enabled = true;
        voice->loop_end = (loop_start_samples + loop_length_samples) << 16;
    }
}

void tracker_voice_reset_position(TrackerVoice* voice) {
    voice->sample_pos = 0;
}

int32_t tracker_voice_get_sample(TrackerVoice* voice) {
    if (!voice->waveform || voice->length == 0) {
        return 0;
    }

    // Check if past loop end
    if (voice->sample_pos >= voice->loop_end) {
        if (voice->loop_enabled) {
            // Wrap to loop start
            uint32_t loop_len = voice->loop_end - voice->loop_start;
            if (loop_len > 0) {
                voice->sample_pos = voice->loop_start + ((voice->sample_pos - voice->loop_start) % loop_len);
            } else {
                voice->sample_pos = voice->loop_start;
            }
        } else {
            // One-shot sample finished - return silence
            return 0;
        }
    }

    // Get sample at integer position (16.16 fixed-point)
    uint32_t pos = voice->sample_pos >> 16;
    int32_t sample;

    if (voice->bit_depth == 16) {
        const int16_t* waveform16 = (const int16_t*)voice->waveform;
        sample = waveform16[pos];
    } else {
        const int8_t* waveform8 = (const int8_t*)voice->waveform;
        sample = waveform8[pos];
    }

    // Advance position
    voice->sample_pos += voice->delta;

    return sample;
}

int32_t tracker_voice_get_sample_scaled(TrackerVoice* voice) {
    int32_t sample = tracker_voice_get_sample(voice);
    return sample * voice->volume;
}

void tracker_voice_get_stereo_sample(TrackerVoice* voice,
                                     int32_t* out_left,
                                     int32_t* out_right) {
    if (!voice->waveform || voice->length == 0) {
        *out_left = 0;
        *out_right = 0;
        return;
    }

    // Wrap position if needed
    if (voice->sample_pos >= voice->length) {
        voice->sample_pos -= voice->length;
    }

    // Get sample
    uint32_t pos = voice->sample_pos >> 16;
    int32_t sample;

    if (voice->bit_depth == 16) {
        const int16_t* waveform16 = (const int16_t*)voice->waveform;
        sample = waveform16[pos];
    } else {
        const int8_t* waveform8 = (const int8_t*)voice->waveform;
        sample = waveform8[pos];
    }

    // Apply volume
    int32_t scaled = sample * voice->volume;

    // Apply panning
    *out_left = (scaled * voice->pan_left) >> 7;
    *out_right = (scaled * voice->pan_right) >> 7;

    // Advance position
    voice->sample_pos += voice->delta;
}
