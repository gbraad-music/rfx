/*
 * Simple Frequency Shifter (Bode-style)
 * Uses Hilbert transform + quadrature oscillator
 */

#include "fx_freqshift.h"
#include "windows_compat.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

struct FXFreqShift {
    int enabled;
    float freq;      // 0.0-1.0 (maps to -500Hz to +500Hz)
    float mix;       // 0.0-1.0 (dry/wet)

    // Per-channel state
    float phase[2];  // oscillator phase (stereo)

    // Hilbert transform state (2 all-pass filters per channel)
    float ap1[2], ap2[2];
};

FXFreqShift* fx_freqshift_create(void)
{
    FXFreqShift* fx = calloc(1, sizeof(FXFreqShift));
    if (!fx) return NULL;

    fx->enabled = 0;
    fx->freq = 0.5f;  // 0 Hz
    fx->mix = 1.0f;   // 100% wet

    return fx;
}

void fx_freqshift_destroy(FXFreqShift* fx)
{
    if (fx) free(fx);
}

void fx_freqshift_reset(FXFreqShift* fx)
{
    if (!fx) return;

    for (int ch = 0; ch < 2; ch++) {
        fx->phase[ch] = 0.0f;
        fx->ap1[ch] = 0.0f;
        fx->ap2[ch] = 0.0f;
    }
}

static inline float allpass(float x, float* z, float c)
{
    float y = c * (x - *z) + *z;
    *z = y;
    return y;
}

static inline float process_channel(FXFreqShift* fx, float x, int channel, int sr)
{
    if (!fx->enabled) return x;

    // Convert parameter to Hz (-500 to +500)
    float freq_hz = (fx->freq - 0.5f) * 1000.0f;

    // --- 1. Hilbert transform (approx) ---
    // Two all-pass filters with tuned coefficients
    float x90 = allpass(x, &fx->ap1[channel], 0.6413f);
    x90 = allpass(x90, &fx->ap2[channel], 0.9260f);

    // x  = original signal (0°)
    // x90 = 90° shifted signal

    // --- 2. Quadrature oscillator ---
    float phaseInc = 2.0f * (float)M_PI * freq_hz / (float)sr;
    fx->phase[channel] += phaseInc;
    if (fx->phase[channel] > (float)M_PI) fx->phase[channel] -= 2.0f * (float)M_PI;
    if (fx->phase[channel] < -(float)M_PI) fx->phase[channel] += 2.0f * (float)M_PI;

    float osc_cos = cosf(fx->phase[channel]);
    float osc_sin = sinf(fx->phase[channel]);

    // --- 3. Ring modulation ---
    float mod1 = x   * osc_cos;
    float mod2 = x90 * osc_sin;

    // --- 4. Single-sideband output ---
    float out = mod1 - mod2;   // upper sideband
    // float out = mod1 + mod2; // lower sideband

    // Mix dry/wet
    return x * (1.0f - fx->mix) + out * fx->mix;
}

void fx_freqshift_process_frame(FXFreqShift* fx, float* left, float* right, int sample_rate)
{
    if (!fx) return;

    *left  = process_channel(fx, *left, 0, sample_rate);
    *right = process_channel(fx, *right, 1, sample_rate);
}

void fx_freqshift_process_f32(FXFreqShift* fx, float* buffer, int frames, int sample_rate)
{
    if (!fx) return;

    for (int i = 0; i < frames; i++) {
        float left  = buffer[i * 2 + 0];
        float right = buffer[i * 2 + 1];

        fx_freqshift_process_frame(fx, &left, &right, sample_rate);

        buffer[i * 2 + 0] = left;
        buffer[i * 2 + 1] = right;
    }
}

void fx_freqshift_process_i16(FXFreqShift* fx, int16_t* buffer, int frames, int sample_rate)
{
    if (!fx) return;

    for (int i = 0; i < frames; i++) {
        float left  = buffer[i * 2 + 0] / 32768.0f;
        float right = buffer[i * 2 + 1] / 32768.0f;

        fx_freqshift_process_frame(fx, &left, &right, sample_rate);

        buffer[i * 2 + 0] = (int16_t)(left  * 32767.0f);
        buffer[i * 2 + 1] = (int16_t)(right * 32767.0f);
    }
}

// Parameter setters
void fx_freqshift_set_enabled(FXFreqShift* fx, int enabled) {
    if (fx) fx->enabled = enabled;
}

void fx_freqshift_set_freq(FXFreqShift* fx, float freq) {
    if (fx) fx->freq = freq;
}

void fx_freqshift_set_mix(FXFreqShift* fx, float mix) {
    if (fx) fx->mix = mix;
}

// Parameter getters
int fx_freqshift_get_enabled(FXFreqShift* fx) {
    return fx ? fx->enabled : 0;
}

float fx_freqshift_get_freq(FXFreqShift* fx) {
    return fx ? fx->freq : 0.5f;
}

float fx_freqshift_get_mix(FXFreqShift* fx) {
    return fx ? fx->mix : 1.0f;
}
