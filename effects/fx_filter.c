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

    // Chamberlin state-variable filter (from regroove_effects.c)
    // Linear cutoff mapping for predictable response
    float nyquist = sample_rate * 0.5f;
    float freq = fx->cutoff * nyquist * 0.48f;
    float f = 2.0f * sinf(3.14159265f * freq / (float)sample_rate);

    // Resonance (Q) - limit range for stability
    // 0.0 resonance = q of 0.7 (gentle)
    // 1.0 resonance = q of 0.1 (strong but stable)
    float q = 0.7f - fx->resonance * 0.6f;
    if (q < 0.1f) q = 0.1f;

    // Process left channel
    fx->lp[0] += f * fx->bp[0];
    float hp = *left - fx->lp[0] - q * fx->bp[0];
    fx->bp[0] += f * hp;
    *left = fx->lp[0];

    // Process right channel
    fx->lp[1] += f * fx->bp[1];
    hp = *right - fx->lp[1] - q * fx->bp[1];
    fx->bp[1] += f * hp;
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
