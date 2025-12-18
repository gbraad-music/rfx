/*
 * MODEL 1 Contour LPF - Low Pass Filter
 * Copyright (C) 2024
 */

#include "fx_model1_lpf.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define LPF_Q 0.5f  // Low Q for non-resonant, transparent sound

struct FXModel1LPF {
    int enabled;
    float cutoff;  // 0.0-1.0 (0.0=500Hz, 1.0=FLAT/20kHz)

    // Biquad coefficients
    float b0, b1, b2;
    float a1, a2;

    // Biquad state (stereo)
    float z1_l, z2_l;
    float z1_r, z2_r;

    int coeffs_dirty;
    int last_sample_rate;
};

static void calculate_coefficients(FXModel1LPF* fx, int sample_rate) {
    if (!fx->coeffs_dirty && sample_rate == fx->last_sample_rate) {
        return;
    }

    // Map cutoff: 0.0 = 500Hz, 1.0 = 20kHz (FLAT)
    // Use exponential curve for natural feel
    float freq_hz = 500.0f * powf(40.0f, fx->cutoff);  // 500Hz to 20kHz
    if (freq_hz > 20000.0f) freq_hz = 20000.0f;
    if (freq_hz < 500.0f) freq_hz = 500.0f;

    // Butterworth lowpass coefficients (RBJ cookbook)
    float omega = 2.0f * M_PI * freq_hz / (float)sample_rate;
    float cos_omega = cosf(omega);
    float sin_omega = sinf(omega);
    float alpha = sin_omega / (2.0f * LPF_Q);

    // Lowpass coefficients
    float _b0 = (1.0f - cos_omega) / 2.0f;
    float _b1 = 1.0f - cos_omega;
    float _b2 = (1.0f - cos_omega) / 2.0f;
    float a0 = 1.0f + alpha;
    float _a1 = -2.0f * cos_omega;
    float _a2 = 1.0f - alpha;

    // Normalize coefficients
    fx->b0 = _b0 / a0;
    fx->b1 = _b1 / a0;
    fx->b2 = _b2 / a0;
    fx->a1 = _a1 / a0;
    fx->a2 = _a2 / a0;

    fx->coeffs_dirty = 0;
    fx->last_sample_rate = sample_rate;
}

FXModel1LPF* fx_model1_lpf_create(void) {
    FXModel1LPF* fx = (FXModel1LPF*)malloc(sizeof(FXModel1LPF));
    if (!fx) return NULL;

    fx->enabled = 0;
    fx->cutoff = 1.0f;  // Default to FLAT (20kHz)

    fx->b0 = 1.0f;
    fx->b1 = 0.0f;
    fx->b2 = 0.0f;
    fx->a1 = 0.0f;
    fx->a2 = 0.0f;

    fx->z1_l = fx->z2_l = 0.0f;
    fx->z1_r = fx->z2_r = 0.0f;

    fx->coeffs_dirty = 1;
    fx->last_sample_rate = 0;

    return fx;
}

void fx_model1_lpf_destroy(FXModel1LPF* fx) {
    if (fx) free(fx);
}

void fx_model1_lpf_reset(FXModel1LPF* fx) {
    if (!fx) return;
    fx->z1_l = fx->z2_l = 0.0f;
    fx->z1_r = fx->z2_r = 0.0f;
    fx->coeffs_dirty = 1;  // Force coefficient recalculation
}

void fx_model1_lpf_process_frame(FXModel1LPF* fx, float* left, float* right, int sample_rate) {
    if (!fx || !fx->enabled) return;

    calculate_coefficients(fx, sample_rate);

    // Process left channel (Direct Form II Transposed)
    float input_l = *left;
    float output_l = fx->b0 * input_l + fx->z1_l;
    fx->z1_l = fx->b1 * input_l - fx->a1 * output_l + fx->z2_l;
    fx->z2_l = fx->b2 * input_l - fx->a2 * output_l;
    *left = output_l;

    // Process right channel
    float input_r = *right;
    float output_r = fx->b0 * input_r + fx->z1_r;
    fx->z1_r = fx->b1 * input_r - fx->a1 * output_r + fx->z2_r;
    fx->z2_r = fx->b2 * input_r - fx->a2 * output_r;
    *right = output_r;
}

void fx_model1_lpf_process_f32(FXModel1LPF* fx, float* left, float* right, int frames, int sample_rate) {
    if (!fx || !fx->enabled) return;

    for (int i = 0; i < frames; i++) {
        fx_model1_lpf_process_frame(fx, &left[i], &right[i], sample_rate);
    }
}

void fx_model1_lpf_process_interleaved(FXModel1LPF* fx, float* buffer, int frames, int sample_rate) {
    if (!fx || !fx->enabled) return;

    for (int i = 0; i < frames; i++) {
        float left = buffer[i * 2];
        float right = buffer[i * 2 + 1];

        fx_model1_lpf_process_frame(fx, &left, &right, sample_rate);

        buffer[i * 2] = left;
        buffer[i * 2 + 1] = right;
    }
}

void fx_model1_lpf_process_i16(FXModel1LPF* fx, int16_t* buffer, int frames, int sample_rate) {
    if (!fx || !fx->enabled) return;

    for (int i = 0; i < frames; i++) {
        float left = buffer[i * 2] / 32768.0f;
        float right = buffer[i * 2 + 1] / 32768.0f;

        fx_model1_lpf_process_frame(fx, &left, &right, sample_rate);

        buffer[i * 2] = (int16_t)(left * 32767.0f);
        buffer[i * 2 + 1] = (int16_t)(right * 32767.0f);
    }
}

void fx_model1_lpf_set_enabled(FXModel1LPF* fx, int enabled) {
    if (fx) fx->enabled = enabled;
}

void fx_model1_lpf_set_cutoff(FXModel1LPF* fx, float cutoff) {
    if (!fx) return;
    if (cutoff < 0.0f) cutoff = 0.0f;
    if (cutoff > 1.0f) cutoff = 1.0f;
    fx->cutoff = cutoff;
    fx->coeffs_dirty = 1;
}

int fx_model1_lpf_get_enabled(FXModel1LPF* fx) {
    return fx ? fx->enabled : 0;
}

float fx_model1_lpf_get_cutoff(FXModel1LPF* fx) {
    return fx ? fx->cutoff : 1.0f;
}
