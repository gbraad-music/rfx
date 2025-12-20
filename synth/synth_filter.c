/*
 * Regroove Synthesizer Filter Implementation
 * TB303-style resonant low-pass filter using state-variable topology
 */

#include "synth_filter.h"
#include <stdlib.h>
#include <math.h>

struct SynthFilter {
    SynthFilterType type;
    float cutoff;
    float resonance;

    // State-variable filter state
    float lp;  // Low-pass output
    float bp;  // Band-pass output
    float hp;  // High-pass output
};

SynthFilter* synth_filter_create(void)
{
    SynthFilter* filter = (SynthFilter*)malloc(sizeof(SynthFilter));
    if (!filter) return NULL;

    filter->type = SYNTH_FILTER_LPF;
    filter->cutoff = 0.5f;
    filter->resonance = 0.0f;

    synth_filter_reset(filter);
    return filter;
}

void synth_filter_destroy(SynthFilter* filter)
{
    if (filter) free(filter);
}

void synth_filter_reset(SynthFilter* filter)
{
    if (!filter) return;
    filter->lp = 0.0f;
    filter->bp = 0.0f;
    filter->hp = 0.0f;
}

void synth_filter_set_type(SynthFilter* filter, SynthFilterType type)
{
    if (filter) filter->type = type;
}

void synth_filter_set_cutoff(SynthFilter* filter, float cutoff)
{
    if (!filter) return;
    filter->cutoff = cutoff < 0.0f ? 0.0f : (cutoff > 1.0f ? 1.0f : cutoff);
}

void synth_filter_set_resonance(SynthFilter* filter, float resonance)
{
    if (!filter) return;
    filter->resonance = resonance < 0.0f ? 0.0f : (resonance > 1.0f ? 1.0f : resonance);
}

float synth_filter_process(SynthFilter* filter, float input, int sample_rate)
{
    if (!filter) return input;

    // Calculate filter coefficients
    // TB303-style: exponential cutoff mapping for more musical control
    float nyquist = sample_rate * 0.5f;

    // Exponential mapping: more range in low frequencies
    float cutoff_exp = filter->cutoff * filter->cutoff * filter->cutoff;
    float freq = cutoff_exp * nyquist * 0.48f;

    // Calculate frequency coefficient with stability limit
    float f = 2.0f * sinf(M_PI * freq / (float)sample_rate);

    // Limit f to prevent instability at high frequencies
    if (f > 1.0f) f = 1.0f;

    // TB303-style resonance: allow self-oscillation at high values
    // Map 0.0-1.0 to a safer Q range to prevent blow-up
    float q = 1.0f - filter->resonance * 0.90f;  // Reduced from 0.96f
    if (q < 0.05f) q = 0.05f;  // Increased minimum from 0.01f

    // Chamberlin state-variable filter
    // This topology is similar to the TB303's ladder filter
    filter->lp += f * filter->bp;
    filter->hp = input - filter->lp - q * filter->bp;
    filter->bp += f * filter->hp;

    // CRITICAL: Clamp ALL state variables to prevent blow-up
    // This is essential for filter stability at high resonance
    if (filter->lp > 2.0f) filter->lp = 2.0f;
    if (filter->lp < -2.0f) filter->lp = -2.0f;
    if (filter->bp > 2.0f) filter->bp = 2.0f;
    if (filter->bp < -2.0f) filter->bp = -2.0f;
    if (filter->hp > 2.0f) filter->hp = 2.0f;
    if (filter->hp < -2.0f) filter->hp = -2.0f;

    // Return the appropriate filter output
    switch (filter->type) {
    case SYNTH_FILTER_LPF:
        return filter->lp;
    case SYNTH_FILTER_HPF:
        return filter->hp;
    case SYNTH_FILTER_BPF:
        return filter->bp;
    default:
        return filter->lp;
    }
}
