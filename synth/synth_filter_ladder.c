/*
 * Regroove Moog Ladder Filter Implementation
 * Simplified 4-pole resonant lowpass filter
 */

#include "synth_filter_ladder.h"
#include <stdlib.h>
#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

struct SynthFilterLadder {
    float cutoff;      // 0.0 to 1.0
    float resonance;   // 0.0 to 1.0

    // 4-stage filter state
    float stage[4];    // Output of each filter stage

    // For thermal compensation and stability
    float feedback;
};

SynthFilterLadder* synth_filter_ladder_create(void)
{
    SynthFilterLadder* filter = (SynthFilterLadder*)malloc(sizeof(SynthFilterLadder));
    if (!filter) return NULL;

    filter->cutoff = 0.5f;
    filter->resonance = 0.0f;

    for (int i = 0; i < 4; i++) {
        filter->stage[i] = 0.0f;
    }
    filter->feedback = 0.0f;

    return filter;
}

void synth_filter_ladder_destroy(SynthFilterLadder* filter)
{
    if (filter) free(filter);
}

void synth_filter_ladder_reset(SynthFilterLadder* filter)
{
    if (!filter) return;

    for (int i = 0; i < 4; i++) {
        filter->stage[i] = 0.0f;
    }
    filter->feedback = 0.0f;
}

void synth_filter_ladder_set_cutoff(SynthFilterLadder* filter, float cutoff)
{
    if (!filter) return;

    // Clamp to valid range
    if (cutoff < 0.0f) cutoff = 0.0f;
    if (cutoff > 1.0f) cutoff = 1.0f;

    filter->cutoff = cutoff;
}

void synth_filter_ladder_set_resonance(SynthFilterLadder* filter, float resonance)
{
    if (!filter) return;

    // Clamp to valid range
    if (resonance < 0.0f) resonance = 0.0f;
    if (resonance > 1.0f) resonance = 1.0f;

    filter->resonance = resonance;
}

// Soft clipping/saturation function (tanh approximation)
static inline float soft_clip(float x)
{
    // Fast tanh approximation for soft clipping
    if (x < -3.0f) return -1.0f;
    if (x > 3.0f) return 1.0f;
    return x * (27.0f + x * x) / (27.0f + 9.0f * x * x);
}

float synth_filter_ladder_process(SynthFilterLadder* filter, float input, int sample_rate)
{
    if (!filter) return 0.0f;

    // Map cutoff (0-1) to filter coefficient
    // Use exponential mapping for musical response
    float cutoff_hz = 20.0f * powf(1000.0f, filter->cutoff); // 20Hz to 20kHz
    float fc = cutoff_hz / (float)sample_rate;

    // Prevent cutoff from going too high (Nyquist limit)
    if (fc > 0.45f) fc = 0.45f;

    // Calculate filter coefficient (bilinear transform approximation)
    float f = 2.0f * sinf(M_PI * fc);
    if (f > 1.0f) f = 1.0f;

    // Resonance with compensation (4.0 is empirical for self-oscillation at resonance=1.0)
    float res = filter->resonance * 4.0f;

    // Resonance compensation to maintain output level
    float res_comp = 1.0f + filter->resonance * 0.5f;

    // Feedback from output (stage 4) back to input
    float feedback_amount = res * filter->feedback;

    // Input with feedback and soft clipping
    float in = soft_clip(input - feedback_amount);

    // 4-stage ladder filter (each stage is a 1-pole lowpass)
    filter->stage[0] += f * (in - filter->stage[0]);
    filter->stage[1] += f * (filter->stage[0] - filter->stage[1]);
    filter->stage[2] += f * (filter->stage[1] - filter->stage[2]);
    filter->stage[3] += f * (filter->stage[2] - filter->stage[3]);

    // Store feedback for next sample
    filter->feedback = filter->stage[3];

    // Output from last stage with resonance compensation
    float output = filter->stage[3] * res_comp;

    // Safety clamp to prevent blow-up
    if (output > 2.0f) output = 2.0f;
    if (output < -2.0f) output = -2.0f;

    return output;
}
