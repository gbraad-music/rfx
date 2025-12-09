/*
 * MODEL 1 Contour HPF - High Pass Filter
 * Copyright (C) 2024
 */

#include "fx_model1_hpf.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define HPF_Q 0.5f  // Low Q for non-resonant, transparent sound

struct FXModel1HPF {
    int enabled;
    float cutoff;  // 0.0-1.0 (0.0=FLAT/20Hz, 1.0=1kHz)

    // Biquad coefficients
    float b0, b1, b2;
    float a1, a2;

    // Biquad state (stereo)
    float z1_l, z2_l;
    float z1_r, z2_r;

    int coeffs_dirty;
    int last_sample_rate;
};

static void calculate_coefficients(FXModel1HPF* fx, int sample_rate) {
    if (!fx->coeffs_dirty && sample_rate == fx->last_sample_rate) {
        return;
    }

    // Map cutoff: 0.0 = 20Hz (FLAT), 1.0 = 1kHz
    // Use exponential curve for natural feel
    float freq_hz = 20.0f * powf(50.0f, fx->cutoff);  // 20Hz to 1kHz
    if (freq_hz > 1000.0f) freq_hz = 1000.0f;
    if (freq_hz < 20.0f) freq_hz = 20.0f;

    // Butterworth highpass coefficients (RBJ cookbook)
    float omega = 2.0f * M_PI * freq_hz / (float)sample_rate;
    float cos_omega = cosf(omega);
    float sin_omega = sinf(omega);
    float alpha = sin_omega / (2.0f * HPF_Q);

    // Highpass coefficients
    float _b0 = (1.0f + cos_omega) / 2.0f;
    float _b1 = -(1.0f + cos_omega);
    float _b2 = (1.0f + cos_omega) / 2.0f;
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

FXModel1HPF* fx_model1_hpf_create(void) {
    FXModel1HPF* fx = (FXModel1HPF*)malloc(sizeof(FXModel1HPF));
    if (!fx) return NULL;

    fx->enabled = 0;
    fx->cutoff = 0.0f;  // Default to FLAT (20Hz)

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

void fx_model1_hpf_destroy(FXModel1HPF* fx) {
    if (fx) free(fx);
}

void fx_model1_hpf_reset(FXModel1HPF* fx) {
    if (!fx) return;
    fx->z1_l = fx->z2_l = 0.0f;
    fx->z1_r = fx->z2_r = 0.0f;
}

void fx_model1_hpf_process_frame(FXModel1HPF* fx, float* left, float* right, int sample_rate) {
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

void fx_model1_hpf_process_f32(FXModel1HPF* fx, float* left, float* right, int frames, int sample_rate) {
    if (!fx || !fx->enabled) return;

    for (int i = 0; i < frames; i++) {
        fx_model1_hpf_process_frame(fx, &left[i], &right[i], sample_rate);
    }
}

void fx_model1_hpf_process_i16(FXModel1HPF* fx, int16_t* buffer, int frames, int sample_rate) {
    if (!fx || !fx->enabled) return;

    for (int i = 0; i < frames; i++) {
        float left = buffer[i * 2] / 32768.0f;
        float right = buffer[i * 2 + 1] / 32768.0f;

        fx_model1_hpf_process_frame(fx, &left, &right, sample_rate);

        buffer[i * 2] = (int16_t)(left * 32767.0f);
        buffer[i * 2 + 1] = (int16_t)(right * 32767.0f);
    }
}

void fx_model1_hpf_set_enabled(FXModel1HPF* fx, int enabled) {
    if (fx) fx->enabled = enabled;
}

void fx_model1_hpf_set_cutoff(FXModel1HPF* fx, float cutoff) {
    if (!fx) return;
    if (cutoff < 0.0f) cutoff = 0.0f;
    if (cutoff > 1.0f) cutoff = 1.0f;
    fx->cutoff = cutoff;
    fx->coeffs_dirty = 1;
}

int fx_model1_hpf_get_enabled(FXModel1HPF* fx) {
    return fx ? fx->enabled : 0;
}

float fx_model1_hpf_get_cutoff(FXModel1HPF* fx) {
    return fx ? fx->cutoff : 0.0f;
}
