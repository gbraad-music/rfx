/**
 * Sample Effects Implementation
 * Granular pitch shifting and time-stretching for sample playback
 * Core algorithm shared with fx_pitchshift.c via fx_granular_shared.h
 */

#include "sample_fx.h"
#include "../effects/fx_granular_shared.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

// Buffer/grain configuration (shared with fx_pitchshift)
#define SAMPLE_FX_BUFFER_SIZE 8192   // Smaller buffer for per-sample processing
#define SAMPLE_FX_GRAIN_SIZE  512    // Grain size in samples
#define SAMPLE_FX_HOP_SIZE    128    // Hop between grains
#define SAMPLE_FX_NUM_GRAINS  4      // Max concurrent grains

typedef struct {
    bool active;
    float read_pos;     // Position in delay buffer
    float phase;        // 0.0 to 1.0 across grain
} SampleGrain;

struct SampleFX {
    // Parameters
    float pitch_semitones;   // -12.0 to +12.0
    float time_stretch;      // 0.5 to 2.0
    float formant;           // 0.0 to 1.0 (unused)

    uint32_t sample_rate;

    // Circular delay buffer (mono, int16)
    int16_t delay_buffer[SAMPLE_FX_BUFFER_SIZE];

    // Hann window for grains
    float window[SAMPLE_FX_GRAIN_SIZE];

    // Active grains
    SampleGrain grains[SAMPLE_FX_NUM_GRAINS];

    int write_pos;
    int hop_counter;
};

// ============================================================================
// Shared Granular Processing Functions (via fx_granular_shared.h)
// ============================================================================

// These functions are now inlined from fx_granular_shared.h:
// - granular_init_hann_window()
// - granular_wrap_position()
// - granular_read_int16()

static void spawn_grain(SampleFX* fx, float start_read_pos) {
    // Find an inactive grain slot
    int gsel = -1;
    for (int g = 0; g < SAMPLE_FX_NUM_GRAINS; g++) {
        if (!fx->grains[g].active) {
            gsel = g;
            break;
        }
    }

    if (gsel < 0) {
        // All grains active, recycle oldest (first)
        gsel = 0;
    }

    SampleGrain* gr = &fx->grains[gsel];
    gr->active = true;
    gr->phase = 0.0f;
    gr->read_pos = start_read_pos;
}

// ============================================================================
// Public API
// ============================================================================

SampleFX* sample_fx_create(uint32_t sample_rate) {
    SampleFX* fx = (SampleFX*)calloc(1, sizeof(SampleFX));
    if (!fx) return NULL;

    fx->pitch_semitones = 0.0f;
    fx->time_stretch = 1.0f;
    fx->formant = 0.5f;
    fx->sample_rate = sample_rate;

    granular_init_hann_window(fx->window, SAMPLE_FX_GRAIN_SIZE);
    sample_fx_reset(fx);

    return fx;
}

void sample_fx_destroy(SampleFX* fx) {
    if (fx) free(fx);
}

void sample_fx_reset(SampleFX* fx) {
    if (!fx) return;

    memset(fx->delay_buffer, 0, sizeof(fx->delay_buffer));

    fx->write_pos = 0;
    fx->hop_counter = 0;

    for (int g = 0; g < SAMPLE_FX_NUM_GRAINS; g++) {
        fx->grains[g].active = false;
        fx->grains[g].read_pos = 0.0f;
        fx->grains[g].phase = 0.0f;
    }
}

void sample_fx_set_pitch(SampleFX* fx, float semitones) {
    if (!fx) return;
    // Clamp to -12 to +12 semitones
    if (semitones < -12.0f) semitones = -12.0f;
    if (semitones > 12.0f) semitones = 12.0f;
    fx->pitch_semitones = semitones;
}

void sample_fx_set_time_stretch(SampleFX* fx, float ratio) {
    if (!fx) return;
    // Clamp to 0.5x to 2.0x
    if (ratio < 0.5f) ratio = 0.5f;
    if (ratio > 2.0f) ratio = 2.0f;
    fx->time_stretch = ratio;
}

void sample_fx_set_formant(SampleFX* fx, float preserve) {
    if (!fx) return;
    fx->formant = preserve;
}

float sample_fx_get_pitch(SampleFX* fx) {
    return fx ? fx->pitch_semitones : 0.0f;
}

float sample_fx_get_time_stretch(SampleFX* fx) {
    return fx ? fx->time_stretch : 1.0f;
}

bool sample_fx_is_active(SampleFX* fx) {
    if (!fx) return false;
    return (fabsf(fx->pitch_semitones) > 0.01f) || (fabsf(fx->time_stretch - 1.0f) > 0.01f);
}

int16_t sample_fx_process_sample(SampleFX* fx, int16_t input) {
    if (!fx) return input;

    // Write input to delay buffer
    fx->delay_buffer[fx->write_pos] = input;

    // Bypass if no effects active
    if (!sample_fx_is_active(fx)) {
        fx->write_pos = (fx->write_pos + 1) % SAMPLE_FX_BUFFER_SIZE;
        return input;
    }

    // PITCH: affects READ rate through grains (2^(semitones/12))
    // Higher pitch = read faster through grain = higher frequency
    float pitch_ratio = powf(2.0f, fx->pitch_semitones / 12.0f);

    // TIME: affects OUTPUT rate (hop size / grain advance rate)
    // time_stretch < 1.0 = faster output (smaller hop)
    // time_stretch > 1.0 = slower output (larger hop)
    float effective_hop = SAMPLE_FX_HOP_SIZE * fx->time_stretch;

    // Spawn new grain periodically (based on time stretch)
    fx->hop_counter++;
    if (fx->hop_counter >= (int)effective_hop) {
        fx->hop_counter = 0;

        // Start grain with look-behind into delay buffer
        float base_delay = (float)(SAMPLE_FX_GRAIN_SIZE * 2);
        float start_pos = (float)fx->write_pos - base_delay;
        spawn_grain(fx, start_pos);
    }

    // Accumulate output from all active grains
    float output = 0.0f;

    for (int g = 0; g < SAMPLE_FX_NUM_GRAINS; g++) {
        SampleGrain* gr = &fx->grains[g];
        if (!gr->active) continue;

        float phase = gr->phase;
        if (phase >= 1.0f) {
            gr->active = false;
            continue;
        }

        // Window position (0 to GRAIN_SIZE-1)
        float f_idx = phase * (float)(SAMPLE_FX_GRAIN_SIZE - 1);
        int i_idx = (int)f_idx;
        float window_val = fx->window[i_idx];

        // Read position in delay buffer - PITCH affects read rate
        // Higher pitch = faster read through grain = divide by ratio
        float read_pos = gr->read_pos + f_idx * (1.0f / pitch_ratio);

        float sample = granular_read_int16(fx->delay_buffer, read_pos, SAMPLE_FX_BUFFER_SIZE);
        output += sample * window_val;

        // Advance grain phase (fixed rate - TIME affects grain spawning, not phase)
        gr->phase += 1.0f / (float)SAMPLE_FX_GRAIN_SIZE;

        if (gr->phase >= 1.0f) {
            gr->active = false;
        }
    }

    // Advance write position
    fx->write_pos = (fx->write_pos + 1) % SAMPLE_FX_BUFFER_SIZE;

    // Clamp output to int16 range
    if (output > 32767.0f) output = 32767.0f;
    if (output < -32768.0f) output = -32768.0f;

    return (int16_t)output;
}

void sample_fx_process_buffer(SampleFX* fx, int16_t* buffer, uint32_t num_samples) {
    if (!fx || !buffer) return;

    for (uint32_t i = 0; i < num_samples; i++) {
        buffer[i] = sample_fx_process_sample(fx, buffer[i]);
    }
}
