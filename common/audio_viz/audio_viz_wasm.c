/**
 * Audio Visualization WASM Bindings
 * Exposes shared C code (vu_meter.h, waveform.h) to JavaScript
 *
 * Copyright (C) 2025
 * SPDX-License-Identifier: ISC
 */

#include <emscripten.h>
#include <stdint.h>
#include <stdlib.h>
#include "vu_meter.h"
#include "waveform.h"

// ===== VU METER API =====

EMSCRIPTEN_KEEPALIVE
VUMeter* vu_meter_create(uint32_t sample_rate, VUMeterMode mode) {
    VUMeter* vu = (VUMeter*)malloc(sizeof(VUMeter));
    if (vu) {
        vu_meter_init(vu, sample_rate, mode);
    }
    return vu;
}

EMSCRIPTEN_KEEPALIVE
void vu_meter_destroy_wasm(VUMeter* vu) {
    if (vu) {
        free(vu);
    }
}

EMSCRIPTEN_KEEPALIVE
void vu_meter_process_sample(VUMeter* vu, float left, float right) {
    if (vu) {
        vu_meter_process(vu, left, right);
    }
}

EMSCRIPTEN_KEEPALIVE
void vu_meter_update_peaks(VUMeter* vu, float left_peak, float right_peak, float time_delta_ms) {
    if (!vu) return;

    // Update peak values directly (already computed by worklet)
    // Instant attack
    if (left_peak > vu->peak_left) {
        vu->peak_left = left_peak;
        vu->peak_hold_timer_left = (uint32_t)(vu->peak_hold_time * vu->sample_rate);
    }
    if (right_peak > vu->peak_right) {
        vu->peak_right = right_peak;
        vu->peak_hold_timer_right = (uint32_t)(vu->peak_hold_time * vu->sample_rate);
    }

    // Apply decay based on time elapsed
    // decay_rate is in dB/second, so convert time_delta_ms to seconds
    float time_delta_sec = time_delta_ms / 1000.0f;
    float decay_db = vu->decay_rate * time_delta_sec;

    // Convert current peak to dB, apply decay, convert back
    if (vu->peak_left > 0.00001f) {
        float current_db = 20.0f * log10f(vu->peak_left);
        float decayed_db = current_db - decay_db;
        vu->peak_left = powf(10.0f, decayed_db / 20.0f);
        if (vu->peak_left < 0.00001f) vu->peak_left = 0.0f;
    }

    if (vu->peak_right > 0.00001f) {
        float current_db = 20.0f * log10f(vu->peak_right);
        float decayed_db = current_db - decay_db;
        vu->peak_right = powf(10.0f, decayed_db / 20.0f);
        if (vu->peak_right < 0.00001f) vu->peak_right = 0.0f;
    }

    // Update peak hold
    if (vu->peak_hold_time > 0.0f) {
        // Update peak hold values
        if (vu->peak_left > vu->peak_hold_left) {
            vu->peak_hold_left = vu->peak_left;
            vu->peak_hold_timer_left = (uint32_t)(vu->peak_hold_time * vu->sample_rate);
        }
        if (vu->peak_right > vu->peak_hold_right) {
            vu->peak_hold_right = vu->peak_right;
            vu->peak_hold_timer_right = (uint32_t)(vu->peak_hold_time * vu->sample_rate);
        }

        // Decrement hold timers based on time elapsed
        uint32_t samples_elapsed = (uint32_t)(time_delta_sec * vu->sample_rate);
        if (vu->peak_hold_timer_left > samples_elapsed) {
            vu->peak_hold_timer_left -= samples_elapsed;
        } else {
            vu->peak_hold_timer_left = 0;
            // Slow decay of peak hold
            vu->peak_hold_left *= powf(0.9999f, samples_elapsed);
            if (vu->peak_hold_left < 0.00001f) vu->peak_hold_left = 0.0f;
        }

        if (vu->peak_hold_timer_right > samples_elapsed) {
            vu->peak_hold_timer_right -= samples_elapsed;
        } else {
            vu->peak_hold_timer_right = 0;
            vu->peak_hold_right *= powf(0.9999f, samples_elapsed);
            if (vu->peak_hold_right < 0.00001f) vu->peak_hold_right = 0.0f;
        }
    }

    // Convert to dB for display
    vu->peak_left_db = (vu->peak_left > 0.00001f) ? 20.0f * log10f(vu->peak_left) : -96.0f;
    vu->peak_right_db = (vu->peak_right > 0.00001f) ? 20.0f * log10f(vu->peak_right) : -96.0f;
    vu->peak_hold_left_db = (vu->peak_hold_left > 0.00001f) ? 20.0f * log10f(vu->peak_hold_left) : -96.0f;
    vu->peak_hold_right_db = (vu->peak_hold_right > 0.00001f) ? 20.0f * log10f(vu->peak_hold_right) : -96.0f;
}

EMSCRIPTEN_KEEPALIVE
float vu_meter_get_peak_left_db(VUMeter* vu) {
    return vu ? vu_meter_get_peak_db(vu, 0) : -100.0f;
}

EMSCRIPTEN_KEEPALIVE
float vu_meter_get_peak_right_db(VUMeter* vu) {
    return vu ? vu_meter_get_peak_db(vu, 1) : -100.0f;
}

EMSCRIPTEN_KEEPALIVE
float vu_meter_get_peak_hold_left_db(VUMeter* vu) {
    return vu ? vu_meter_get_peak_hold_db(vu, 0) : -100.0f;
}

EMSCRIPTEN_KEEPALIVE
float vu_meter_get_peak_hold_right_db(VUMeter* vu) {
    return vu ? vu_meter_get_peak_hold_db(vu, 1) : -100.0f;
}

EMSCRIPTEN_KEEPALIVE
float vu_meter_get_rms_left_db(VUMeter* vu) {
    return vu ? vu_meter_get_rms_db(vu, 0) : -100.0f;
}

EMSCRIPTEN_KEEPALIVE
float vu_meter_get_rms_right_db(VUMeter* vu) {
    return vu ? vu_meter_get_rms_db(vu, 1) : -100.0f;
}

EMSCRIPTEN_KEEPALIVE
void vu_meter_reset_peaks(VUMeter* vu) {
    if (vu) {
        vu_meter_reset(vu);
    }
}

// ===== WAVEFORM API =====

EMSCRIPTEN_KEEPALIVE
WaveformDisplay* waveform_create(uint32_t buffer_size, WaveformChannelMode channel_mode, uint32_t sample_rate) {
    WaveformDisplay* wf = (WaveformDisplay*)malloc(sizeof(WaveformDisplay));
    if (wf) {
        if (waveform_init(wf, buffer_size, channel_mode, sample_rate) != 0) {
            free(wf);
            return NULL;
        }
    }
    return wf;
}

EMSCRIPTEN_KEEPALIVE
void waveform_destroy_wasm(WaveformDisplay* wf) {
    if (wf) {
        waveform_destroy(wf);
        free(wf);
    }
}

EMSCRIPTEN_KEEPALIVE
void waveform_write_mono_sample(WaveformDisplay* wf, float sample) {
    if (wf) {
        waveform_write_mono(wf, &sample, 1);
    }
}

EMSCRIPTEN_KEEPALIVE
void waveform_write_stereo_sample(WaveformDisplay* wf, float left, float right) {
    if (wf) {
        float samples[2] = {left, right};
        waveform_write_stereo(wf, samples, 1);
    }
}

EMSCRIPTEN_KEEPALIVE
void waveform_write_mono_buffer(WaveformDisplay* wf, const float* samples, uint32_t num_samples) {
    if (wf && samples) {
        waveform_write_mono(wf, samples, num_samples);
    }
}

EMSCRIPTEN_KEEPALIVE
void waveform_write_stereo_buffer(WaveformDisplay* wf, const float* left_samples, const float* right_samples, uint32_t num_samples) {
    if (wf && left_samples && right_samples) {
        waveform_write_separate(wf, left_samples, right_samples, num_samples);
    }
}

EMSCRIPTEN_KEEPALIVE
float* waveform_get_buffer_left(WaveformDisplay* wf) {
    return wf ? wf->buffer_left : NULL;
}

EMSCRIPTEN_KEEPALIVE
float* waveform_get_buffer_right(WaveformDisplay* wf) {
    return wf ? wf->buffer_right : NULL;
}

EMSCRIPTEN_KEEPALIVE
uint32_t waveform_get_buffer_size(WaveformDisplay* wf) {
    return wf ? wf->buffer_size : 0;
}

EMSCRIPTEN_KEEPALIVE
uint32_t waveform_get_write_position(WaveformDisplay* wf) {
    return wf ? wf->write_position : 0;
}

EMSCRIPTEN_KEEPALIVE
void waveform_clear_wasm(WaveformDisplay* wf) {
    if (wf) {
        waveform_clear(wf);
    }
}

// ===== HELPER FUNCTIONS (enum getters) =====

EMSCRIPTEN_KEEPALIVE
int get_vu_meter_mode_peak(void) {
    return VU_MODE_PEAK;
}

EMSCRIPTEN_KEEPALIVE
int get_vu_meter_mode_rms(void) {
    return VU_MODE_RMS;
}

EMSCRIPTEN_KEEPALIVE
int get_vu_meter_mode_vu_classic(void) {
    return VU_MODE_VU_CLASSIC;
}

EMSCRIPTEN_KEEPALIVE
int get_vu_meter_mode_ppm(void) {
    return VU_MODE_PPM;
}

EMSCRIPTEN_KEEPALIVE
int get_waveform_channel_mono(void) {
    return WAVEFORM_CHANNEL_MONO;
}

EMSCRIPTEN_KEEPALIVE
int get_waveform_channel_stereo(void) {
    return WAVEFORM_CHANNEL_STEREO;
}

EMSCRIPTEN_KEEPALIVE
int get_waveform_channel_mid_side(void) {
    return WAVEFORM_CHANNEL_MID_SIDE;
}
