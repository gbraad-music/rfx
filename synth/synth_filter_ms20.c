/*
 * Regroove MS-20 Style Dual Filter Implementation
 * HPF + LPF cascade with aggressive resonance
 */

#include "synth_filter_ms20.h"
#include <stdlib.h>
#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#ifndef M_2PI
#define M_2PI 6.28318530717958647692
#endif

struct SynthFilterMS20 {
    // HPF state
    float hpf_cutoff;
    float hpf_peak;
    float hpf_lp;
    float hpf_bp;
    float hpf_hp;

    // LPF state
    float lpf_cutoff;
    float lpf_peak;
    float lpf_lp;
    float lpf_bp;
    float lpf_hp;
};

SynthFilterMS20* synth_filter_ms20_create(void)
{
    SynthFilterMS20* filter = (SynthFilterMS20*)malloc(sizeof(SynthFilterMS20));
    if (!filter) return NULL;

    filter->hpf_cutoff = 0.1f;
    filter->hpf_peak = 0.0f;
    filter->hpf_lp = 0.0f;
    filter->hpf_bp = 0.0f;
    filter->hpf_hp = 0.0f;

    filter->lpf_cutoff = 0.8f;
    filter->lpf_peak = 0.0f;
    filter->lpf_lp = 0.0f;
    filter->lpf_bp = 0.0f;
    filter->lpf_hp = 0.0f;

    return filter;
}

void synth_filter_ms20_destroy(SynthFilterMS20* filter)
{
    if (filter) free(filter);
}

void synth_filter_ms20_reset(SynthFilterMS20* filter)
{
    if (!filter) return;

    filter->hpf_lp = 0.0f;
    filter->hpf_bp = 0.0f;
    filter->hpf_hp = 0.0f;

    filter->lpf_lp = 0.0f;
    filter->lpf_bp = 0.0f;
    filter->lpf_hp = 0.0f;
}

void synth_filter_ms20_set_hpf_cutoff(SynthFilterMS20* filter, float cutoff)
{
    if (!filter) return;
    if (cutoff < 0.0f) cutoff = 0.0f;
    if (cutoff > 1.0f) cutoff = 1.0f;
    filter->hpf_cutoff = cutoff;
}

void synth_filter_ms20_set_hpf_peak(SynthFilterMS20* filter, float peak)
{
    if (!filter) return;
    if (peak < 0.0f) peak = 0.0f;
    if (peak > 1.0f) peak = 1.0f;
    filter->hpf_peak = peak;
}

void synth_filter_ms20_set_lpf_cutoff(SynthFilterMS20* filter, float cutoff)
{
    if (!filter) return;
    if (cutoff < 0.0f) cutoff = 0.0f;
    if (cutoff > 1.0f) cutoff = 1.0f;
    filter->lpf_cutoff = cutoff;
}

void synth_filter_ms20_set_lpf_peak(SynthFilterMS20* filter, float peak)
{
    if (!filter) return;
    if (peak < 0.0f) peak = 0.0f;
    if (peak > 1.0f) peak = 1.0f;
    filter->lpf_peak = peak;
}

// Soft saturation/clipping for aggressive MS-20 character
static inline float ms20_saturate(float x)
{
    // Asymmetric soft clipping (MS-20 has diode clipping)
    if (x > 1.0f) {
        return 1.0f + (x - 1.0f) * 0.3f;
    } else if (x < -1.0f) {
        return -1.0f + (x + 1.0f) * 0.3f;
    }
    return x;
}

// Tanh approximation for additional saturation
static inline float fast_tanh(float x)
{
    if (x < -3.0f) return -1.0f;
    if (x > 3.0f) return 1.0f;
    return x * (27.0f + x * x) / (27.0f + 9.0f * x * x);
}

float synth_filter_ms20_process(SynthFilterMS20* filter, float input, int sample_rate)
{
    if (!filter) return 0.0f;

    float sample = input;

    // === HIGHPASS FILTER ===
    // Map cutoff to frequency coefficient (20Hz to 8kHz range for HPF)
    float hpf_hz = 20.0f + filter->hpf_cutoff * 7980.0f;
    float hpf_fc = hpf_hz / (float)sample_rate;
    if (hpf_fc > 0.45f) hpf_fc = 0.45f;

    float hpf_f = 2.0f * sinf(M_PI * hpf_fc);
    if (hpf_f > 1.0f) hpf_f = 1.0f;

    // MS-20 style aggressive resonance (can self-oscillate at peak=1.0)
    float hpf_q = 1.0f - filter->hpf_peak * 0.98f;
    if (hpf_q < 0.01f) hpf_q = 0.01f;

    // State variable filter with saturation
    sample = ms20_saturate(sample);

    filter->hpf_lp += hpf_f * filter->hpf_bp;
    filter->hpf_hp = sample - filter->hpf_lp - hpf_q * filter->hpf_bp;
    filter->hpf_bp += hpf_f * filter->hpf_hp;

    // Apply saturation to filter states (MS-20 characteristic)
    filter->hpf_bp = fast_tanh(filter->hpf_bp * 1.5f);

    // Clamp states to prevent blow-up
    if (filter->hpf_lp > 3.0f) filter->hpf_lp = 3.0f;
    if (filter->hpf_lp < -3.0f) filter->hpf_lp = -3.0f;
    if (filter->hpf_bp > 3.0f) filter->hpf_bp = 3.0f;
    if (filter->hpf_bp < -3.0f) filter->hpf_bp = -3.0f;
    if (filter->hpf_hp > 3.0f) filter->hpf_hp = 3.0f;
    if (filter->hpf_hp < -3.0f) filter->hpf_hp = -3.0f;

    // Output highpass
    sample = filter->hpf_hp;

    // === LOWPASS FILTER ===
    // Map cutoff to frequency coefficient (50Hz to 20kHz range for LPF)
    float lpf_hz = 50.0f * powf(400.0f, filter->lpf_cutoff); // 50Hz to 20kHz
    float lpf_fc = lpf_hz / (float)sample_rate;
    if (lpf_fc > 0.45f) lpf_fc = 0.45f;

    float lpf_f = 2.0f * sinf(M_PI * lpf_fc);
    if (lpf_f > 1.0f) lpf_f = 1.0f;

    // MS-20 style aggressive resonance
    float lpf_q = 1.0f - filter->lpf_peak * 0.98f;
    if (lpf_q < 0.01f) lpf_q = 0.01f;

    // State variable filter with saturation
    sample = ms20_saturate(sample);

    filter->lpf_lp += lpf_f * filter->lpf_bp;
    filter->lpf_hp = sample - filter->lpf_lp - lpf_q * filter->lpf_bp;
    filter->lpf_bp += lpf_f * filter->lpf_hp;

    // Apply saturation to filter states
    filter->lpf_bp = fast_tanh(filter->lpf_bp * 1.5f);

    // Clamp states to prevent blow-up
    if (filter->lpf_lp > 3.0f) filter->lpf_lp = 3.0f;
    if (filter->lpf_lp < -3.0f) filter->lpf_lp = -3.0f;
    if (filter->lpf_bp > 3.0f) filter->lpf_bp = 3.0f;
    if (filter->lpf_bp < -3.0f) filter->lpf_bp = -3.0f;
    if (filter->lpf_hp > 3.0f) filter->lpf_hp = 3.0f;
    if (filter->lpf_hp < -3.0f) filter->lpf_hp = -3.0f;

    // Output lowpass
    sample = filter->lpf_lp;

    // Final safety clamp
    if (sample > 2.0f) sample = 2.0f;
    if (sample < -2.0f) sample = -2.0f;

    return sample;
}
