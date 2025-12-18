/*
 * Regroove Resonant Filter for logue SDK
 *
 * DJ-style resonant low-pass filter
 * Compatible with: minilogue xd, prologue, NTS-1
 */

#include "userfx.h"
#include <math.h>

typedef struct {
    float cutoff;      // 0.0 - 1.0 (normalized frequency)
    float resonance;   // 0.0 - 1.0 (Q factor)

    // Filter state (stereo)
    float lp[2];       // Low-pass state
    float bp[2];       // Band-pass state
} RegrooveFilter;

static RegrooveFilter fx_state;

void FX_INIT(uint32_t platform, uint32_t api)
{
    fx_state.cutoff = 0.8f;      // Default to mostly open
    fx_state.resonance = 0.3f;   // Moderate resonance

    for (int i = 0; i < 2; i++) {
        fx_state.lp[i] = 0.0f;
        fx_state.bp[i] = 0.0f;
    }
}

void FX_PROCESS(float *xn, uint32_t frames)
{
    const float cutoff = fx_state.cutoff;
    const float resonance = fx_state.resonance;

    // Map cutoff to frequency (20Hz - 20kHz, exponential)
    // cutoff=0 -> 20Hz, cutoff=1 -> 20kHz
    const float min_freq = 20.0f / 48000.0f;
    const float max_freq = 20000.0f / 48000.0f;
    const float freq = min_freq * fx_pow(max_freq / min_freq, cutoff);

    // Map resonance to Q (0.5 - 20.0)
    const float q = 0.5f + resonance * 19.5f;

    float * __restrict x = xn;
    const float * x_e = x + (frames << 1);

    for (; x != x_e; ) {
        // Left channel
        float input = *x;
        float notch = input - fx_state.bp[0] * q;
        fx_state.lp[0] = fx_state.lp[0] + freq * fx_state.bp[0];
        fx_state.bp[0] = fx_state.bp[0] + freq * notch;
        *x++ = fx_state.lp[0];

        // Right channel
        input = *x;
        notch = input - fx_state.bp[1] * q;
        fx_state.lp[1] = fx_state.lp[1] + freq * fx_state.bp[1];
        fx_state.bp[1] = fx_state.bp[1] + freq * notch;
        *x++ = fx_state.lp[1];
    }
}

void FX_PARAM(uint8_t index, int32_t value)
{
    const float valf = param_val_to_f32(value);

    switch (index) {
    case 0: // Cutoff
        fx_state.cutoff = valf;
        break;
    case 1: // Resonance
        fx_state.resonance = valf;
        break;
    default:
        break;
    }
}
