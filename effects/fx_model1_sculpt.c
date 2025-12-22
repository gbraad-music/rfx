/*
 * MODEL 1 Sculpt - Semi-Parametric EQ
 * Copyright (C) 2024
 */

#include "fx_model1_sculpt.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define SCULPT_Q 1.0f  // Wide Q for musical sound

struct FXModel1Sculpt {
    int enabled;
    float frequency;  // 0.0-1.0 (70Hz to 7kHz)
    float gain;       // 0.0-1.0 (0.0=-20dB, 0.5=0dB, 1.0=+8dB)

    // Biquad coefficients
    float b0, b1, b2;
    float a1, a2;

    // Biquad state (stereo)
    float x1_l, x2_l;
    float x1_r, x2_r;

    int coeffs_dirty;
    int last_sample_rate;
};

static void calculate_coefficients(FXModel1Sculpt* fx, int sample_rate) {
    if (!fx->coeffs_dirty && sample_rate == fx->last_sample_rate) {
        return;
    }

    // Map frequency: 0.0 = 70Hz, 1.0 = 7kHz (logarithmic)
    // 70Hz to 7kHz is 100:1 ratio (100x = 2^6.644 ~= 7000/70)
    float freq_hz = 70.0f * powf(100.0f, fx->frequency);
    if (freq_hz > 7000.0f) freq_hz = 7000.0f;
    if (freq_hz < 70.0f) freq_hz = 70.0f;

    // Map gain: 0.0 = -20dB, 0.5 = 0dB, 1.0 = +8dB (asymmetric)
    float gain_db;
    if (fx->gain < 0.5f) {
        // Cut range: 0.0 to 0.5 maps to -20dB to 0dB
        gain_db = (fx->gain - 0.5f) * 40.0f;  // -20 to 0
    } else {
        // Boost range: 0.5 to 1.0 maps to 0dB to +8dB
        gain_db = (fx->gain - 0.5f) * 16.0f;  // 0 to +8
    }

    // Calculate biquad coefficients (RBJ cookbook peaking EQ)
    float omega = 2.0f * M_PI * freq_hz / (float)sample_rate;
    float cos_omega = cosf(omega);
    float sin_omega = sinf(omega);

    float A = powf(10.0f, gain_db / 40.0f);  // Amplitude from dB
    float alpha = sin_omega / (2.0f * SCULPT_Q);

    // Peaking EQ coefficients
    float _b0 = 1.0f + alpha * A;
    float _b1 = -2.0f * cos_omega;
    float _b2 = 1.0f - alpha * A;
    float a0 = 1.0f + alpha / A;
    float _a1 = -2.0f * cos_omega;
    float _a2 = 1.0f - alpha / A;

    // Normalize coefficients
    fx->b0 = _b0 / a0;
    fx->b1 = _b1 / a0;
    fx->b2 = _b2 / a0;
    fx->a1 = _a1 / a0;
    fx->a2 = _a2 / a0;

    fx->coeffs_dirty = 0;
    fx->last_sample_rate = sample_rate;
}

FXModel1Sculpt* fx_model1_sculpt_create(void) {
    FXModel1Sculpt* fx = (FXModel1Sculpt*)malloc(sizeof(FXModel1Sculpt));
    if (!fx) return NULL;

    fx->enabled = 0;
    fx->frequency = 0.5f;  // Default to ~500Hz (mid-point)
    fx->gain = 0.5f;       // Default to 0dB (neutral)

    fx->b0 = 1.0f;
    fx->b1 = 0.0f;
    fx->b2 = 0.0f;
    fx->a1 = 0.0f;
    fx->a2 = 0.0f;

    fx->x1_l = fx->x2_l = 0.0f;
    fx->x1_r = fx->x2_r = 0.0f;

    fx->coeffs_dirty = 1;
    fx->last_sample_rate = 0;

    return fx;
}

void fx_model1_sculpt_destroy(FXModel1Sculpt* fx) {
    if (fx) free(fx);
}

void fx_model1_sculpt_reset(FXModel1Sculpt* fx) {
    if (!fx) return;
    fx->x1_l = fx->x2_l = 0.0f;
    fx->x1_r = fx->x2_r = 0.0f;
    fx->coeffs_dirty = 1;  // Force coefficient recalculation
}

void fx_model1_sculpt_process_frame(FXModel1Sculpt* fx, float* left, float* right, int sample_rate) {
    if (!fx || !fx->enabled) return;

    calculate_coefficients(fx, sample_rate);

    // Process left channel (Direct Form II Transposed)
    float input_l = *left;
    float output_l = fx->b0 * input_l + fx->x1_l;
    fx->x1_l = fx->b1 * input_l - fx->a1 * output_l + fx->x2_l;
    fx->x2_l = fx->b2 * input_l - fx->a2 * output_l;
    *left = output_l;

    // Process right channel
    float input_r = *right;
    float output_r = fx->b0 * input_r + fx->x1_r;
    fx->x1_r = fx->b1 * input_r - fx->a1 * output_r + fx->x2_r;
    fx->x2_r = fx->b2 * input_r - fx->a2 * output_r;
    *right = output_r;
}

void fx_model1_sculpt_process_f32(FXModel1Sculpt* fx, float* left, float* right, int frames, int sample_rate) {
    if (!fx || !fx->enabled) return;

    for (int i = 0; i < frames; i++) {
        fx_model1_sculpt_process_frame(fx, &left[i], &right[i], sample_rate);
    }
}

void fx_model1_sculpt_process_interleaved(FXModel1Sculpt* fx, float* buffer, int frames, int sample_rate) {
    if (!fx || !fx->enabled) return;

    for (int i = 0; i < frames; i++) {
        float left = buffer[i * 2];
        float right = buffer[i * 2 + 1];

        fx_model1_sculpt_process_frame(fx, &left, &right, sample_rate);

        buffer[i * 2] = left;
        buffer[i * 2 + 1] = right;
    }
}

void fx_model1_sculpt_process_i16(FXModel1Sculpt* fx, int16_t* buffer, int frames, int sample_rate) {
    if (!fx || !fx->enabled) return;

    for (int i = 0; i < frames; i++) {
        float left = buffer[i * 2] / 32768.0f;
        float right = buffer[i * 2 + 1] / 32768.0f;

        fx_model1_sculpt_process_frame(fx, &left, &right, sample_rate);

        buffer[i * 2] = (int16_t)(left * 32767.0f);
        buffer[i * 2 + 1] = (int16_t)(right * 32767.0f);
    }
}

void fx_model1_sculpt_set_enabled(FXModel1Sculpt* fx, int enabled) {
    if (fx) fx->enabled = enabled;
}

void fx_model1_sculpt_set_frequency(FXModel1Sculpt* fx, float freq) {
    if (!fx) return;
    if (freq < 0.0f) freq = 0.0f;
    if (freq > 1.0f) freq = 1.0f;
    fx->frequency = freq;
    fx->coeffs_dirty = 1;
}

void fx_model1_sculpt_set_gain(FXModel1Sculpt* fx, float gain) {
    if (!fx) return;
    if (gain < 0.0f) gain = 0.0f;
    if (gain > 1.0f) gain = 1.0f;
    fx->gain = gain;
    fx->coeffs_dirty = 1;
}

int fx_model1_sculpt_get_enabled(FXModel1Sculpt* fx) {
    return fx ? fx->enabled : 0;
}

float fx_model1_sculpt_get_frequency(FXModel1Sculpt* fx) {
    return fx ? fx->frequency : 0.5f;
}

float fx_model1_sculpt_get_gain(FXModel1Sculpt* fx) {
    return fx ? fx->gain : 0.5f;
}

// ============================================================================
// Generic Parameter Interface
// ============================================================================

#include "../param_interface.h"

typedef enum {
    FX_MODEL1_SCULPT_GROUP_MAIN = 0,
    FX_MODEL1_SCULPT_GROUP_COUNT
} FXModel1SculptParamGroup;

typedef enum {
    FX_MODEL1_SCULPT_PARAM_FREQUENCY = 0,
    FX_MODEL1_SCULPT_PARAM_GAIN,
    FX_MODEL1_SCULPT_PARAM_COUNT
} FXModel1SculptParamIndex;

// Parameter metadata (ALL VALUES NORMALIZED 0.0-1.0)
static const ParameterInfo model1_sculpt_params[FX_MODEL1_SCULPT_PARAM_COUNT] = {
    {"Frequency", "Hz", 0.5f, 0.0f, 1.0f, FX_MODEL1_SCULPT_GROUP_MAIN, 0},
    {"Gain", "dB", 0.5f, 0.0f, 1.0f, FX_MODEL1_SCULPT_GROUP_MAIN, 0}
};

static const char* group_names[FX_MODEL1_SCULPT_GROUP_COUNT] = {"Sculpt"};

int fx_model1_sculpt_get_parameter_count(void) { return FX_MODEL1_SCULPT_PARAM_COUNT; }

float fx_model1_sculpt_get_parameter_value(FXModel1Sculpt* fx, int index) {
    if (!fx || index < 0 || index >= FX_MODEL1_SCULPT_PARAM_COUNT) return 0.0f;
    switch (index) {
        case FX_MODEL1_SCULPT_PARAM_FREQUENCY: return fx_model1_sculpt_get_frequency(fx);
        case FX_MODEL1_SCULPT_PARAM_GAIN: return fx_model1_sculpt_get_gain(fx);
        default: return 0.0f;
    }
}

void fx_model1_sculpt_set_parameter_value(FXModel1Sculpt* fx, int index, float value) {
    if (!fx || index < 0 || index >= FX_MODEL1_SCULPT_PARAM_COUNT) return;
    switch (index) {
        case FX_MODEL1_SCULPT_PARAM_FREQUENCY: fx_model1_sculpt_set_frequency(fx, value); break;
        case FX_MODEL1_SCULPT_PARAM_GAIN: fx_model1_sculpt_set_gain(fx, value); break;
    }
}

DEFINE_PARAM_METADATA_ACCESSORS(fx_model1_sculpt, model1_sculpt_params, FX_MODEL1_SCULPT_PARAM_COUNT, group_names, FX_MODEL1_SCULPT_GROUP_COUNT)
