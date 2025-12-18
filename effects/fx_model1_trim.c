/*
 * FX-Model1 Trim/Drive Effect Implementation
 */

#include "fx_model1_trim.h"
#include <stdlib.h>
#include <math.h>

struct FXModel1Trim {
    int enabled;
    float drive; // 0.0 to 1.0
    float peak_level; // Peak output level (0.0 to 1.0+)
};

FXModel1Trim* fx_model1_trim_create(void) {
    FXModel1Trim* fx = (FXModel1Trim*)malloc(sizeof(FXModel1Trim));
    if (!fx) return NULL;
    fx->enabled = 0;
    fx->drive = 0.7f;  // Default to unity gain (0dB)
    fx->peak_level = 0.0f;
    return fx;
}

void fx_model1_trim_destroy(FXModel1Trim* fx) {
    if (fx) free(fx);
}

void fx_model1_trim_reset(FXModel1Trim* fx) {
    if (fx) fx->drive = 0.0f;
}

// Fast tanh approximation for soft clipping
static inline float fast_tanh(float x) {
    x = fmaxf(-3.0f, fminf(x, 3.0f));
    float x2 = x * x;
    return x * (27.0f + x2) / (27.0f + 9.0f * x2);
}

static inline float apply_trim_drive(float sample, float drive) {
    // Proper TRIM control:
    // 0.0 (0%) = -18dB attenuation (0.125x gain)
    // 0.7 (70%) = 0dB unity gain (1.0x gain)
    // 1.0 (100%) = +6dB with drive (2.0x gain with soft clipping)

    float gain;
    if (drive < 0.7f) {
        // Attenuation range: 0% to 70% maps to 0.125x to 1.0x gain
        gain = 0.125f + (drive / 0.7f) * (1.0f - 0.125f);
        return sample * gain;
    } else {
        // Drive range: 70% to 100% maps to 1.0x to 2.0x gain with soft clipping
        // Use exponential curve for last 30% to make drive kick in strongly at the end
        float drive_amount = (drive - 0.7f) / 0.3f;  // Normalize to 0-1
        gain = 1.0f + drive_amount * drive_amount * drive_amount * 1.0f;  // cubic for gentle start, strong finish
        float driven_sample = sample * gain;

        // Apply soft clipping for analog-style overdrive
        return fast_tanh(driven_sample);
    }
}

void fx_model1_trim_process_frame(FXModel1Trim* fx, float* left, float* right, int sample_rate) {
    if (!fx || !fx->enabled) return;
    (void)sample_rate; // Unused for this effect

    *left = apply_trim_drive(*left, fx->drive);
    *right = apply_trim_drive(*right, fx->drive);
}

void fx_model1_trim_process_interleaved(FXModel1Trim* fx, float* buffer, int frames, int sample_rate) {
    if (!fx || !fx->enabled) return;
    (void)sample_rate; // Unused for this effect

    // Reset peak at start of buffer
    float peak = 0.0f;

    // Process interleaved stereo buffer and track peak
    for (int i = 0; i < frames; i++) {
        buffer[i * 2] = apply_trim_drive(buffer[i * 2], fx->drive);
        buffer[i * 2 + 1] = apply_trim_drive(buffer[i * 2 + 1], fx->drive);

        // Track peak output level
        float absL = fabsf(buffer[i * 2]);
        float absR = fabsf(buffer[i * 2 + 1]);
        float sample_peak = (absL > absR) ? absL : absR;
        if (sample_peak > peak) {
            peak = sample_peak;
        }
    }

    // Peak hold with decay (smooth LED response)
    const float decay_rate = 0.95f;
    if (peak > fx->peak_level) {
        fx->peak_level = peak;
    } else {
        fx->peak_level *= decay_rate;
    }
}

void fx_model1_trim_process_f32(FXModel1Trim* fx, float* buffer, int frames, int sample_rate) {
    if (!fx || !fx->enabled) return;
    (void)sample_rate; // Unused for this effect

    // Reset peak at start of buffer
    float peak = 0.0f;

    // Process single channel buffer and track peak
    for (int i = 0; i < frames; i++) {
        buffer[i] = apply_trim_drive(buffer[i], fx->drive);
        float sample_peak = fabsf(buffer[i]);
        if (sample_peak > peak) {
            peak = sample_peak;
        }
    }

    // Peak hold with decay
    const float decay_rate = 0.95f;
    if (peak > fx->peak_level) {
        fx->peak_level = peak;
    } else {
        fx->peak_level *= decay_rate;
    }
}

void fx_model1_trim_set_drive(FXModel1Trim* fx, float drive) {
    if (fx) fx->drive = fmaxf(0.0f, fminf(drive, 1.0f));
}

float fx_model1_trim_get_drive(FXModel1Trim* fx) {
    return fx ? fx->drive : 0.0f;
}

void fx_model1_trim_set_enabled(FXModel1Trim* fx, int enabled) {
    if (fx) fx->enabled = enabled;
}

int fx_model1_trim_get_enabled(FXModel1Trim* fx) {
    return fx ? fx->enabled : 0;
}

float fx_model1_trim_get_peak_level(FXModel1Trim* fx) {
    return fx ? fx->peak_level : 0.0f;
}
