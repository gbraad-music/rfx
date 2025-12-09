/*
 * Regroove Resonant Filter Implementation
 */

#include "fx_filter.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

struct FXFilter {
    // Parameters
    int enabled;
    float cutoff;      // 0.0 - 1.0 (normalized frequency)
    float resonance;   // 0.0 - 1.0 (Q factor)

    // Filter state (stereo)
    float lp[2];       // Low-pass state
    float bp[2];       // Band-pass state
};

FXFilter* fx_filter_create(void)
{
    FXFilter* fx = (FXFilter*)malloc(sizeof(FXFilter));
    if (!fx) return NULL;

    fx->enabled = 0;
    fx->cutoff = 0.8f;
    fx->resonance = 0.3f;

    fx_filter_reset(fx);
    return fx;
}

void fx_filter_destroy(FXFilter* fx)
{
    if (fx) free(fx);
}

void fx_filter_reset(FXFilter* fx)
{
    if (!fx) return;

    for (int i = 0; i < 2; i++) {
        fx->lp[i] = 0.0f;
        fx->bp[i] = 0.0f;
    }
}

void fx_filter_process_frame(FXFilter* fx, float* left, float* right, int sample_rate)
{
    if (!fx || !fx->enabled) return;

    // Map cutoff to frequency (20Hz - 20kHz, exponential)
    const float min_freq = 20.0f / sample_rate;
    const float max_freq = 20000.0f / sample_rate;
    const float freq = min_freq * powf(max_freq / min_freq, fx->cutoff);

    // Map resonance to Q (0.5 - 20.0)
    const float q = 0.5f + fx->resonance * 19.5f;

    // Left channel
    float notch_l = *left - fx->bp[0] * q;
    fx->lp[0] = fx->lp[0] + freq * fx->bp[0];
    fx->bp[0] = fx->bp[0] + freq * notch_l;
    *left = fx->lp[0];

    // Right channel
    float notch_r = *right - fx->bp[1] * q;
    fx->lp[1] = fx->lp[1] + freq * fx->bp[1];
    fx->bp[1] = fx->bp[1] + freq * notch_r;
    *right = fx->lp[1];
}

void fx_filter_process_f32(FXFilter* fx, float* buffer, int frames, int sample_rate)
{
    if (!fx || !fx->enabled) return;

    for (int i = 0; i < frames; i++) {
        fx_filter_process_frame(fx, &buffer[i * 2], &buffer[i * 2 + 1], sample_rate);
    }
}

void fx_filter_process_i16(FXFilter* fx, int16_t* buffer, int frames, int sample_rate)
{
    if (!fx || !fx->enabled) return;

    for (int i = 0; i < frames; i++) {
        float left = buffer[i * 2] / 32768.0f;
        float right = buffer[i * 2 + 1] / 32768.0f;

        fx_filter_process_frame(fx, &left, &right, sample_rate);

        buffer[i * 2] = (int16_t)(left * 32767.0f);
        buffer[i * 2 + 1] = (int16_t)(right * 32767.0f);
    }
}

void fx_filter_set_enabled(FXFilter* fx, int enabled)
{
    if (fx) fx->enabled = enabled;
}

void fx_filter_set_cutoff(FXFilter* fx, float cutoff)
{
    if (fx) fx->cutoff = cutoff < 0.0f ? 0.0f : (cutoff > 1.0f ? 1.0f : cutoff);
}

void fx_filter_set_resonance(FXFilter* fx, float resonance)
{
    if (fx) fx->resonance = resonance < 0.0f ? 0.0f : (resonance > 1.0f ? 1.0f : resonance);
}

int fx_filter_get_enabled(FXFilter* fx)
{
    return fx ? fx->enabled : 0;
}

float fx_filter_get_cutoff(FXFilter* fx)
{
    return fx ? fx->cutoff : 0.0f;
}

float fx_filter_get_resonance(FXFilter* fx)
{
    return fx ? fx->resonance : 0.0f;
}
