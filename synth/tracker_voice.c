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
}

void tracker_voice_set_waveform(TrackerVoice* voice,
                                const int8_t* waveform,
                                uint32_t length) {
    voice->waveform = waveform;
    voice->length = length << 16;  // Convert to fixed-point
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

void tracker_voice_reset_position(TrackerVoice* voice) {
    voice->sample_pos = 0;
}

int8_t tracker_voice_get_sample(TrackerVoice* voice) {
    if (!voice->waveform || voice->length == 0) {
        return 0;
    }

    // Wrap position if needed
    if (voice->sample_pos >= voice->length) {
        voice->sample_pos -= voice->length;
    }

    // Get sample at integer position (16.16 fixed-point)
    int8_t sample = voice->waveform[voice->sample_pos >> 16];

    // Advance position
    voice->sample_pos += voice->delta;

    return sample;
}

int32_t tracker_voice_get_sample_scaled(TrackerVoice* voice) {
    int8_t sample = tracker_voice_get_sample(voice);
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

    // Get sample and apply volume
    int8_t sample = voice->waveform[voice->sample_pos >> 16];
    int32_t scaled = sample * voice->volume;

    // Apply panning
    *out_left = (scaled * voice->pan_left) >> 7;
    *out_right = (scaled * voice->pan_right) >> 7;

    // Advance position
    voice->sample_pos += voice->delta;
}
