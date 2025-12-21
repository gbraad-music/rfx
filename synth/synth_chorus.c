/*
 * Regroove BBD-Style Chorus Implementation
 */

#include "synth_chorus.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#ifndef M_2PI
#define M_2PI 6.28318530717958647692
#endif

#define MAX_DELAY_MS 50.0f
#define MAX_DELAY_SAMPLES 4410  // 50ms at 88.2kHz max

struct SynthChorus {
    ChorusMode mode;
    float rate;
    float depth;

    // Delay line
    float delay_buffer[MAX_DELAY_SAMPLES];
    int delay_write_pos;

    // LFO for modulation
    float lfo_phase;
    float lfo_phase2;  // For dual chorus (offset phase)
};

SynthChorus* synth_chorus_create(void)
{
    SynthChorus* chorus = (SynthChorus*)malloc(sizeof(SynthChorus));
    if (!chorus) return NULL;

    chorus->mode = CHORUS_OFF;
    chorus->rate = 0.8f;  // Hz
    chorus->depth = 0.5f;

    memset(chorus->delay_buffer, 0, sizeof(chorus->delay_buffer));
    chorus->delay_write_pos = 0;

    chorus->lfo_phase = 0.0f;
    chorus->lfo_phase2 = 0.5f;  // 180 degrees out of phase

    return chorus;
}

void synth_chorus_destroy(SynthChorus* chorus)
{
    if (chorus) free(chorus);
}

void synth_chorus_reset(SynthChorus* chorus)
{
    if (!chorus) return;

    memset(chorus->delay_buffer, 0, sizeof(chorus->delay_buffer));
    chorus->delay_write_pos = 0;
    chorus->lfo_phase = 0.0f;
    chorus->lfo_phase2 = 0.5f;
}

void synth_chorus_set_mode(SynthChorus* chorus, ChorusMode mode)
{
    if (chorus) chorus->mode = mode;
}

void synth_chorus_set_rate(SynthChorus* chorus, float rate_hz)
{
    if (!chorus) return;
    if (rate_hz < 0.1f) rate_hz = 0.1f;
    if (rate_hz > 10.0f) rate_hz = 10.0f;
    chorus->rate = rate_hz;
}

void synth_chorus_set_depth(SynthChorus* chorus, float depth)
{
    if (!chorus) return;
    if (depth < 0.0f) depth = 0.0f;
    if (depth > 1.0f) depth = 1.0f;
    chorus->depth = depth;
}

// Linear interpolation for fractional delay
static float read_delay(SynthChorus* chorus, float delay_samples)
{
    int delay_int = (int)delay_samples;
    float delay_frac = delay_samples - delay_int;

    int read_pos1 = chorus->delay_write_pos - delay_int;
    while (read_pos1 < 0) read_pos1 += MAX_DELAY_SAMPLES;
    read_pos1 = read_pos1 % MAX_DELAY_SAMPLES;

    int read_pos2 = (read_pos1 - 1);
    while (read_pos2 < 0) read_pos2 += MAX_DELAY_SAMPLES;
    read_pos2 = read_pos2 % MAX_DELAY_SAMPLES;

    float sample1 = chorus->delay_buffer[read_pos1];
    float sample2 = chorus->delay_buffer[read_pos2];

    return sample1 + delay_frac * (sample2 - sample1);
}

void synth_chorus_process(SynthChorus* chorus, float input,
                          float* out_left, float* out_right, int sample_rate)
{
    if (!chorus || !out_left || !out_right) {
        if (out_left) *out_left = input;
        if (out_right) *out_right = input;
        return;
    }

    // Write input to delay line
    chorus->delay_buffer[chorus->delay_write_pos] = input;
    chorus->delay_write_pos = (chorus->delay_write_pos + 1) % MAX_DELAY_SAMPLES;

    if (chorus->mode == CHORUS_OFF) {
        *out_left = input;
        *out_right = input;
        return;
    }

    // Update LFO phase
    float phase_inc = chorus->rate / sample_rate;
    chorus->lfo_phase += phase_inc;
    if (chorus->lfo_phase >= 1.0f) chorus->lfo_phase -= 1.0f;

    chorus->lfo_phase2 += phase_inc;
    if (chorus->lfo_phase2 >= 1.0f) chorus->lfo_phase2 -= 1.0f;

    // Generate LFO (sine wave)
    float lfo1 = sinf(M_2PI * chorus->lfo_phase);
    float lfo2 = sinf(M_2PI * chorus->lfo_phase2);

    // Juno-style delay times
    // Chorus I: ~5ms base delay, modulated ±2ms
    // Chorus II: two taps at different depths
    float base_delay_ms = 5.0f;
    float mod_depth_ms = 2.0f * chorus->depth;

    if (chorus->mode == CHORUS_I) {
        // Single delay tap, panned center
        float delay_ms = base_delay_ms + lfo1 * mod_depth_ms;
        float delay_samples = (delay_ms / 1000.0f) * sample_rate;

        if (delay_samples < 1.0f) delay_samples = 1.0f;
        if (delay_samples >= MAX_DELAY_SAMPLES) delay_samples = MAX_DELAY_SAMPLES - 1;

        float delayed = read_delay(chorus, delay_samples);

        // Mix: 50% dry, 50% wet
        *out_left = input * 0.7f + delayed * 0.3f;
        *out_right = input * 0.7f + delayed * 0.3f;
    }
    else if (chorus->mode == CHORUS_II) {
        // Dual delay taps, stereo spread
        float delay1_ms = base_delay_ms + lfo1 * mod_depth_ms;
        float delay2_ms = base_delay_ms + 1.5f + lfo2 * mod_depth_ms * 0.8f;

        float delay1_samples = (delay1_ms / 1000.0f) * sample_rate;
        float delay2_samples = (delay2_ms / 1000.0f) * sample_rate;

        if (delay1_samples < 1.0f) delay1_samples = 1.0f;
        if (delay1_samples >= MAX_DELAY_SAMPLES) delay1_samples = MAX_DELAY_SAMPLES - 1;
        if (delay2_samples < 1.0f) delay2_samples = 1.0f;
        if (delay2_samples >= MAX_DELAY_SAMPLES) delay2_samples = MAX_DELAY_SAMPLES - 1;

        float delayed1 = read_delay(chorus, delay1_samples);
        float delayed2 = read_delay(chorus, delay2_samples);

        // Stereo spread
        *out_left = input * 0.6f + delayed1 * 0.3f + delayed2 * 0.1f;
        *out_right = input * 0.6f + delayed2 * 0.3f + delayed1 * 0.1f;
    }
}
