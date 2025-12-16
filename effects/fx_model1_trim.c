/*
 * FX-Model1 Trim/Drive Effect Implementation
 */

#include "fx_model1_trim.h"
#include <stdlib.h>
#include <math.h>

struct FXModel1Trim {
    float drive; // 0.0 to 1.0
};

FXModel1Trim* fx_model1_trim_create(void) {
    FXModel1Trim* fx = (FXModel1Trim*)malloc(sizeof(FXModel1Trim));
    if (!fx) return NULL;
    fx->drive = 0.0f;
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
    if (drive <= 0.0f) {
        return sample;
    }
    // As drive approaches 1.0, gain increases significantly
    float gain = 1.0f + drive * drive * 50.0f;
    float driven_sample = sample * gain;
    
    // Apply soft clipping for analog-style overdrive
    return fast_tanh(driven_sample);
}

void fx_model1_trim_process_frame(FXModel1Trim* fx, float* left, float* right, int sample_rate) {
    if (!fx || fx->drive <= 0.0f) return;
    (void)sample_rate; // Unused for this effect

    *left = apply_trim_drive(*left, fx->drive);
    *right = apply_trim_drive(*right, fx->drive);
}

void fx_model1_trim_process_f32(FXModel1Trim* fx, float* buffer, int frames, int sample_rate) {
    if (!fx || fx->drive <= 0.0f) return;
    (void)sample_rate; // Unused for this effect

    for (int i = 0; i < frames * 2; i++) {
        buffer[i] = apply_trim_drive(buffer[i], fx->drive);
    }
}

void fx_model1_trim_set_drive(FXModel1Trim* fx, float drive) {
    if (fx) fx->drive = fmaxf(0.0f, fminf(drive, 1.0f));
}

float fx_model1_trim_get_drive(FXModel1Trim* fx) {
    return fx ? fx->drive : 0.0f;
}
