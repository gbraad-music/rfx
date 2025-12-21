/*
 * Regroove Karplus-Strong Synthesis
 * Physical modeling of plucked strings using delay line feedback
 */

#include "synth_karplus.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define MAX_DELAY_SAMPLES 4096  // ~85Hz at 44.1kHz

struct SynthKarplus {
    float buffer[MAX_DELAY_SAMPLES];
    int buffer_size;
    int read_pos;
    int write_pos;

    float damping;
    float brightness;
    float stretch;
    float pick_position;

    // Filter state (one-pole lowpass)
    float filter_z1;

    // Decay envelope for release
    float decay_rate;
    float amplitude;
    bool active;
};

SynthKarplus* synth_karplus_create(void)
{
    SynthKarplus* ks = (SynthKarplus*)malloc(sizeof(SynthKarplus));
    if (!ks) return NULL;

    memset(ks->buffer, 0, sizeof(ks->buffer));
    ks->buffer_size = 0;
    ks->read_pos = 0;
    ks->write_pos = 0;

    ks->damping = 0.5f;
    ks->brightness = 0.5f;
    ks->stretch = 0.0f;
    ks->pick_position = 0.5f;

    ks->filter_z1 = 0.0f;
    ks->decay_rate = 0.0f;
    ks->amplitude = 0.0f;
    ks->active = false;

    return ks;
}

void synth_karplus_destroy(SynthKarplus* ks)
{
    if (ks) free(ks);
}

void synth_karplus_set_damping(SynthKarplus* ks, float damping)
{
    if (ks) {
        if (damping < 0.0f) damping = 0.0f;
        if (damping > 1.0f) damping = 1.0f;
        ks->damping = damping;
    }
}

void synth_karplus_set_brightness(SynthKarplus* ks, float brightness)
{
    if (ks) {
        if (brightness < 0.0f) brightness = 0.0f;
        if (brightness > 1.0f) brightness = 1.0f;
        ks->brightness = brightness;
    }
}

void synth_karplus_set_stretch(SynthKarplus* ks, float stretch)
{
    if (ks) {
        if (stretch < 0.0f) stretch = 0.0f;
        if (stretch > 1.0f) stretch = 1.0f;
        ks->stretch = stretch;
    }
}

void synth_karplus_set_pick_position(SynthKarplus* ks, float position)
{
    if (ks) {
        if (position < 0.0f) position = 0.0f;
        if (position > 1.0f) position = 1.0f;
        ks->pick_position = position;
    }
}

void synth_karplus_trigger(SynthKarplus* ks, float frequency, float velocity, int sample_rate)
{
    if (!ks || frequency <= 0.0f) return;

    // Calculate buffer size (delay time)
    float period = (float)sample_rate / frequency;

    // Apply stretch tuning (slight inharmonicity like real strings)
    period *= 1.0f + ks->stretch * 0.02f;

    ks->buffer_size = (int)period;
    if (ks->buffer_size < 4) ks->buffer_size = 4;
    if (ks->buffer_size >= MAX_DELAY_SAMPLES) ks->buffer_size = MAX_DELAY_SAMPLES - 1;

    // Fill buffer with noise burst (excitation)
    for (int i = 0; i < ks->buffer_size; i++) {
        // Random noise between -1 and 1
        float noise = ((float)rand() / RAND_MAX) * 2.0f - 1.0f;

        // Apply pick position (filters out harmonics)
        // Position of 0.5 = pluck in center (removes even harmonics)
        float pos_factor = 1.0f;
        if (ks->pick_position > 0.01f && ks->pick_position < 0.99f) {
            float phase = (float)i / ks->buffer_size;
            // Simple position filtering
            pos_factor = sinf(M_PI * phase / ks->pick_position);
            if (pos_factor < 0.0f) pos_factor = 0.0f;
        }

        ks->buffer[i] = noise * velocity * pos_factor;
    }

    ks->read_pos = 0;
    ks->write_pos = 0;
    ks->filter_z1 = 0.0f;
    ks->amplitude = 1.0f;
    ks->decay_rate = 0.0f;
    ks->active = true;
}

void synth_karplus_release(SynthKarplus* ks)
{
    if (!ks) return;
    // Set decay rate for gradual release
    ks->decay_rate = 0.0001f;
}

bool synth_karplus_is_active(SynthKarplus* ks)
{
    return ks && ks->active;
}

float synth_karplus_process(SynthKarplus* ks, int sample_rate)
{
    if (!ks || !ks->active || ks->buffer_size == 0) return 0.0f;

    // Read from delay line
    float output = ks->buffer[ks->read_pos];

    // Damping via one-pole lowpass filter
    // brightness controls the cutoff frequency
    float brightness_factor = 0.1f + ks->brightness * 0.89f;  // 0.1 to 0.99

    // Additional damping control
    float damping_factor = 0.9f + ks->damping * 0.09f;  // 0.9 to 0.99

    // One-pole lowpass: y[n] = a * x[n] + (1-a) * y[n-1]
    float filtered = brightness_factor * output + (1.0f - brightness_factor) * ks->filter_z1;
    ks->filter_z1 = filtered;

    // Apply damping (energy loss)
    filtered *= damping_factor;

    // Write back to delay line (feedback)
    ks->buffer[ks->write_pos] = filtered;

    // Advance positions
    ks->read_pos = (ks->read_pos + 1) % ks->buffer_size;
    ks->write_pos = (ks->write_pos + 1) % ks->buffer_size;

    // Apply decay envelope if releasing
    if (ks->decay_rate > 0.0f) {
        ks->amplitude -= ks->decay_rate;
        if (ks->amplitude <= 0.0f) {
            ks->amplitude = 0.0f;
            ks->active = false;
        }
        output *= ks->amplitude;
    }

    // Check if string has decayed to silence
    if (fabsf(output) < 0.0001f && fabsf(filtered) < 0.0001f) {
        ks->active = false;
    }

    return output;
}

void synth_karplus_reset(SynthKarplus* ks)
{
    if (!ks) return;
    memset(ks->buffer, 0, sizeof(ks->buffer));
    ks->read_pos = 0;
    ks->write_pos = 0;
    ks->filter_z1 = 0.0f;
    ks->amplitude = 0.0f;
    ks->active = false;
}
