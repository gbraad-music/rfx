/*
 * Resonator Module - TR-909 style resonant filter
 */

#include "synth_resonator.h"
#include <stdlib.h>
#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

SynthResonator* synth_resonator_create(void)
{
    SynthResonator* r = calloc(1, sizeof(SynthResonator));
    return r;
}

void synth_resonator_destroy(SynthResonator* r)
{
    free(r);
}

void synth_resonator_reset(SynthResonator* r)
{
    if (!r) return;
    r->z1 = 0.0f;
    r->z2 = 0.0f;
}

void synth_resonator_set_params(SynthResonator* r, float f0, float decay, float fs)
{
    if (!r) return;

    float omega = 2.0f * M_PI * f0 / fs;

    // Convert decay time to pole radius
    // decay = time to -60 dB (-60dB = 0.001 amplitude)
    float r_pole = expf(-6.907755f / (decay * fs));

    r->a1 = 2.0f * r_pole * cosf(omega);
    r->a2 = -r_pole * r_pole;

    // Input gain for proper impulse response
    // Compensates for pole magnitude to maintain unity gain at resonance
    r->b0 = (1.0f - r_pole);
}

void synth_resonator_strike(SynthResonator* r, float strength)
{
    // Not used anymore - resonators are excited via process() input
    (void)r;
    (void)strength;
}

float synth_resonator_process(SynthResonator* r, float x)
{
    if (!r) return 0.0f;

    // Standard 2-pole resonator with input
    float y = r->b0 * x + r->a1 * r->z1 + r->a2 * r->z2;

    r->z2 = r->z1;
    r->z1 = y;

    return y;
}
