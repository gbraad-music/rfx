/*
 * Regroove Pitch Shifter Implementation
 * Granular time-domain pitch shifter with overlap-add
 */

#include "fx_pitchshift.h"
#include "windows_compat.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define PITCHSHIFT_BUFFER_SIZE 16384
#define PITCHSHIFT_GRAIN_SIZE 1024
#define PITCHSHIFT_HOP_SIZE   256
#define PITCHSHIFT_NUM_GRAINS 4

typedef struct {
    int   active;
    float read_pos;     // absolute position in delay buffer
    float phase;        // 0..1 position within grain
} FXGrain;

struct FXPitchShift {
    // Parameters
    int enabled;
    float pitch;   // 0.0 - 1.0 maps to -12..+12 semitones
    float mix;     // 0.0 - 1.0 (dry/wet)
    float formant; // 0.0 - 1.0 (unused for now)

    // Circular delay buffer (stereo)
    float delay_buffer[2][PITCHSHIFT_BUFFER_SIZE];

    // Hann window
    float window[PITCHSHIFT_GRAIN_SIZE];

    // Grain state: [channel][grain_index]
    FXGrain grains[2][PITCHSHIFT_NUM_GRAINS];

    int write_pos;
    int hop_counter;
};

static void initialize_window(float* window, int size)
{
    for (int i = 0; i < size; ++i)
        window[i] = 0.5f * (1.0f - cosf(2.0f * (float)M_PI * i / (size - 1)));
}

FXPitchShift* fx_pitchshift_create(void)
{
    FXPitchShift* fx = (FXPitchShift*)calloc(1, sizeof(FXPitchShift));
    if (!fx) return NULL;

    fx->enabled = 0;
    fx->pitch   = 0.5f;  // 0 semitones
    fx->mix     = 1.0f;
    fx->formant = 0.5f;

    initialize_window(fx->window, PITCHSHIFT_GRAIN_SIZE);
    fx_pitchshift_reset(fx);

    return fx;
}

void fx_pitchshift_destroy(FXPitchShift* fx)
{
    if (fx) free(fx);
}

void fx_pitchshift_reset(FXPitchShift* fx)
{
    if (!fx) return;

    memset(fx->delay_buffer, 0, sizeof(fx->delay_buffer));

    fx->write_pos   = 0;
    fx->hop_counter = 0;

    for (int ch = 0; ch < 2; ++ch) {
        for (int g = 0; g < PITCHSHIFT_NUM_GRAINS; ++g) {
            fx->grains[ch][g].active   = 0;
            fx->grains[ch][g].read_pos = 0.0f;
            fx->grains[ch][g].phase    = 0.0f;
        }
    }
}

static inline float wrap_position(float pos)
{
    while (pos < 0.0f)
        pos += (float)PITCHSHIFT_BUFFER_SIZE;
    while (pos >= (float)PITCHSHIFT_BUFFER_SIZE)
        pos -= (float)PITCHSHIFT_BUFFER_SIZE;
    return pos;
}

static inline float read_delay_interpolated(const float* buffer, float position)
{
    position = wrap_position(position);

    int idx0 = (int)position;
    float frac = position - (float)idx0;
    int idx1 = idx0 + 1;
    if (idx1 >= PITCHSHIFT_BUFFER_SIZE) idx1 = 0;

    return buffer[idx0] * (1.0f - frac) + buffer[idx1] * frac;
}

// Spawn a new grain for a channel at a given input "time"
static void spawn_grain(FXPitchShift* fx, int channel, float start_read_pos)
{
    // Find an inactive grain slot
    int gsel = -1;
    for (int g = 0; g < PITCHSHIFT_NUM_GRAINS; ++g) {
        if (!fx->grains[channel][g].active) {
            gsel = g;
            break;
        }
    }
    if (gsel < 0) {
        // If all grains are active, just recycle the first one
        gsel = 0;
    }

    FXGrain* gr = &fx->grains[channel][gsel];
    gr->active   = 1;
    gr->phase    = 0.0f;
    gr->read_pos = start_read_pos;
}

// Process one channel sample
static inline float process_channel(FXPitchShift* fx, float input, int channel)
{
    // Write incoming sample into delay buffer
    fx->delay_buffer[channel][fx->write_pos] = input;

    // If disabled or near 0 semitones, bypass (but still write buffer)
    float semitones = (fx->pitch - 0.5f) * 24.0f;
    if (!fx->enabled || fabsf(semitones) < 0.01f) {
        return input;
    }

    // Pitch ratio
    float ratio = powf(2.0f, semitones / 12.0f);

    // Output accumulator for this sample
    float out = 0.0f;

    // Advance / render all active grains
    for (int g = 0; g < PITCHSHIFT_NUM_GRAINS; ++g) {
        FXGrain* gr = &fx->grains[channel][g];
        if (!gr->active)
            continue;

        // Phase 0..1 across grain
        float phase = gr->phase;
        if (phase >= 1.0f) {
            gr->active = 0;
            continue;
        }

        // Grain index into window
        float f_idx = phase * (float)(PITCHSHIFT_GRAIN_SIZE - 1);
        int   i_idx = (int)f_idx;
        float w     = fx->window[i_idx];

        // Read position in delay buffer, step in "input time" scaled by ratio
        float read_pos = gr->read_pos + f_idx * (1.0f / ratio);

        float sample = read_delay_interpolated(fx->delay_buffer[channel], read_pos);
        out += sample * w;

        // Advance grain phase
        gr->phase += 1.0f / (float)PITCHSHIFT_GRAIN_SIZE;

        if (gr->phase >= 1.0f) {
            gr->active = 0;
        }
    }

    // Grain spawning logic:
    // every HOP_SIZE output samples, we spawn a new grain
    // whose start_read_pos is some fixed delay behind write_pos
    // and advances in input-time according to ratio.
    if (fx->hop_counter == 0) {
        // choose a delay so that we have enough "look-back" for the grain
        float base_delay = (float)(PITCHSHIFT_GRAIN_SIZE * 2);
        float start_read = (float)fx->write_pos - base_delay;
        start_read = wrap_position(start_read);

        spawn_grain(fx, channel, start_read);
    }

    // Mix dry / wet
    float wet = out * fx->mix;
    float dry = input * (1.0f - fx->mix);

    return dry + wet;
}

void fx_pitchshift_process_frame(FXPitchShift* fx, float* left, float* right, int sample_rate)
{
    if (!fx) return;

    *left  = process_channel(fx, *left, 0);
    *right = process_channel(fx, *right, 1);

    fx->write_pos++;
    if (fx->write_pos >= PITCHSHIFT_BUFFER_SIZE)
        fx->write_pos = 0;

    fx->hop_counter++;
    if (fx->hop_counter >= PITCHSHIFT_HOP_SIZE)
        fx->hop_counter = 0;

    (void)sample_rate; // currently unused
}

void fx_pitchshift_process_f32(FXPitchShift* fx, float* buffer, int frames, int sample_rate)
{
    if (!fx) return;

    for (int i = 0; i < frames; ++i) {
        float left  = buffer[i * 2 + 0];
        float right = buffer[i * 2 + 1];

        fx_pitchshift_process_frame(fx, &left, &right, sample_rate);

        buffer[i * 2 + 0] = left;
        buffer[i * 2 + 1] = right;
    }
}

void fx_pitchshift_process_i16(FXPitchShift* fx, int16_t* buffer, int frames, int sample_rate)
{
    if (!fx) return;

    for (int i = 0; i < frames; ++i) {
        float left  = buffer[i * 2 + 0] / 32768.0f;
        float right = buffer[i * 2 + 1] / 32768.0f;

        fx_pitchshift_process_frame(fx, &left, &right, sample_rate);

        buffer[i * 2 + 0] = (int16_t)(left  * 32767.0f);
        buffer[i * 2 + 1] = (int16_t)(right * 32767.0f);
    }
}

// Parameter setters/getters
void fx_pitchshift_set_enabled(FXPitchShift* fx, int enabled) {
    if (fx) fx->enabled = enabled;
}

void fx_pitchshift_set_pitch(FXPitchShift* fx, float pitch) {
    if (fx) fx->pitch = pitch;
}

void fx_pitchshift_set_mix(FXPitchShift* fx, float mix) {
    if (fx) fx->mix = mix;
}

void fx_pitchshift_set_formant(FXPitchShift* fx, float formant) {
    if (fx) fx->formant = formant;
}

int fx_pitchshift_get_enabled(FXPitchShift* fx) {
    return fx ? fx->enabled : 0;
}

float fx_pitchshift_get_pitch(FXPitchShift* fx) {
    return fx ? fx->pitch : 0.5f;
}

float fx_pitchshift_get_mix(FXPitchShift* fx) {
    return fx ? fx->mix : 1.0f;
}

float fx_pitchshift_get_formant(FXPitchShift* fx) {
    return fx ? fx->formant : 0.5f;
}

// ============================================================================
// Generic Parameter Interface
// ============================================================================

#include "../param_interface.h"

typedef enum {
    FX_PITCHSHIFT_GROUP_MAIN = 0,
    FX_PITCHSHIFT_GROUP_COUNT
} FXPitchShiftParamGroup;

typedef enum {
    FX_PITCHSHIFT_PARAM_PITCH = 0,
    FX_PITCHSHIFT_PARAM_MIX,
    FX_PITCHSHIFT_PARAM_FORMANT,
    FX_PITCHSHIFT_PARAM_COUNT
} FXPitchShiftParamIndex;

// Parameter metadata (ALL VALUES NORMALIZED 0.0-1.0)
static const ParameterInfo pitchshift_params[FX_PITCHSHIFT_PARAM_COUNT] = {
    {"Pitch", "st", 0.5f, 0.0f, 1.0f, FX_PITCHSHIFT_GROUP_MAIN, 0},
    {"Mix", "%", 1.0f, 0.0f, 1.0f, FX_PITCHSHIFT_GROUP_MAIN, 0},
    {"Formant", "%", 0.5f, 0.0f, 1.0f, FX_PITCHSHIFT_GROUP_MAIN, 0}
};

static const char* group_names[FX_PITCHSHIFT_GROUP_COUNT] = {"PitchShift"};

int fx_pitchshift_get_parameter_count(void) { return FX_PITCHSHIFT_PARAM_COUNT; }

float fx_pitchshift_get_parameter_value(FXPitchShift* fx, int index) {
    if (!fx || index < 0 || index >= FX_PITCHSHIFT_PARAM_COUNT) return 0.0f;
    switch (index) {
        case FX_PITCHSHIFT_PARAM_PITCH: return fx_pitchshift_get_pitch(fx);
        case FX_PITCHSHIFT_PARAM_MIX: return fx_pitchshift_get_mix(fx);
        case FX_PITCHSHIFT_PARAM_FORMANT: return fx_pitchshift_get_formant(fx);
        default: return 0.0f;
    }
}

void fx_pitchshift_set_parameter_value(FXPitchShift* fx, int index, float value) {
    if (!fx || index < 0 || index >= FX_PITCHSHIFT_PARAM_COUNT) return;
    switch (index) {
        case FX_PITCHSHIFT_PARAM_PITCH: fx_pitchshift_set_pitch(fx, value); break;
        case FX_PITCHSHIFT_PARAM_MIX: fx_pitchshift_set_mix(fx, value); break;
        case FX_PITCHSHIFT_PARAM_FORMANT: fx_pitchshift_set_formant(fx, value); break;
    }
}

DEFINE_PARAM_METADATA_ACCESSORS(fx_pitchshift, pitchshift_params, FX_PITCHSHIFT_PARAM_COUNT, group_names, FX_PITCHSHIFT_GROUP_COUNT)
