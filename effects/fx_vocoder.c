/*
 * Vocoder Effect Implementation
 * Hybrid vocoder with internal/external/MIDI carrier
 * Tuned for classic "robot voice" with modern stability
 */

#include "fx_vocoder.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdbool.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define NUM_BANDS 16
#define MIN_CARRIER_FREQ 50.0f    // Hz
#define MAX_CARRIER_FREQ 500.0f   // Hz

// Biquad bandpass filter
typedef struct {
    float b0, b1, b2, a1, a2;
    float x1, x2, y1, y2;
} BiquadBP;

// Envelope follower
typedef struct {
    float level;
    float attack_coeff;
    float release_coeff;
} EnvelopeFollower;

struct FXVocoder {
    bool enabled;
    float carrier_freq;      // 0.0-1.0
    float carrier_wave;      // 0.0-1.0 (waveform shape)
    float formant_shift;     // 0.0-1.0 (0.5 = neutral)
    float release;           // 0.0-1.0 (controls env release)
    float mix;               // 0.0-1.0

    int carrier_mode;        // VocoderCarrierMode
    int midi_note;           // MIDI note

    // Internal carrier oscillator
    float carrier_phase;

    // Filter bank
    BiquadBP modulator_bands[NUM_BANDS];
    BiquadBP carrier_bands[NUM_BANDS];
    EnvelopeFollower envelopes[NUM_BANDS];

    int sample_rate;
    float last_formant_shift;
    float last_release;
    bool filters_initialized;
};

// ============================================================================
// Filter Bank Frequencies (16 bands, ~80 Hz - ~8 kHz)
// Slightly spaced for vocal formants / intelligibility
// ============================================================================

static const float BAND_FREQUENCIES[NUM_BANDS] = {
    80.0f,   160.0f,  250.0f,  350.0f,
    500.0f,  750.0f, 1100.0f, 1600.0f,
    2300.0f, 3300.0f, 4500.0f, 6000.0f,
    7500.0f, 9000.0f, 11000.0f, 13000.0f
};

// ============================================================================
// Biquad Bandpass Filter
// ============================================================================

static void biquad_bp_init(BiquadBP* bp, float freq, float q, int sample_rate) {
    float omega = 2.0f * M_PI * freq / (float)sample_rate;
    float cos_omega = cosf(omega);
    float sin_omega = sinf(omega);
    float alpha = sin_omega / (2.0f * q);

    float a0 = 1.0f + alpha;
    bp->b0 = alpha / a0;
    bp->b1 = 0.0f;
    bp->b2 = -alpha / a0;
    bp->a1 = -2.0f * cos_omega / a0;
    bp->a2 = (1.0f - alpha) / a0;

    bp->x1 = bp->x2 = 0.0f;
    bp->y1 = bp->y2 = 0.0f;
}

static float biquad_bp_process(BiquadBP* bp, float input) {
    float output =
        bp->b0 * input +
        bp->b1 * bp->x1 +
        bp->b2 * bp->x2 -
        bp->a1 * bp->y1 -
        bp->a2 * bp->y2;

    bp->x2 = bp->x1;
    bp->x1 = input;
    bp->y2 = bp->y1;
    bp->y1 = output;

    return output;
}

// ============================================================================
// Envelope Follower
// Hybrid: fast enough for intelligibility, slow enough for "robot"
// ============================================================================

static void envelope_follower_init(EnvelopeFollower* env, float release_param, int sample_rate) {
    env->level = 0.0f;

    float attack_time  = 0.005f; // 5 ms
    float release_time = 0.050f + release_param * 0.150f; // 50–200 ms

    env->attack_coeff  = expf(-1.0f / (attack_time  * (float)sample_rate));
    env->release_coeff = expf(-1.0f / (release_time * (float)sample_rate));
}

static float envelope_follower_process(EnvelopeFollower* env, float input) {
    float x = fabsf(input);
    if (x > env->level) {
        env->level = x + env->attack_coeff * (env->level - x);
    } else {
        env->level = x + env->release_coeff * (env->level - x);
    }
    return env->level;
}

// ============================================================================
// Carrier Oscillator
// waveform: 0.0=saw, 0.5=square, 1.0=pulse (morphing region)
// ============================================================================

static float generate_carrier(float phase, float waveform) {
    // ignore waveform for now, just saw
    float t = phase / (2.0f * M_PI);
    if (t >= 1.0f) t -= 1.0f;
    return 2.0f * t - 1.0f;
}

// ============================================================================
// Lifecycle
// ============================================================================

FXVocoder* fx_vocoder_create(void) {
    FXVocoder* fx = (FXVocoder*)calloc(1, sizeof(FXVocoder));
    if (!fx) return NULL;

    fx->enabled = false;
    fx->carrier_freq = 0.3f;      // ~200 Hz
    fx->carrier_wave = 0.0f;      // Sawtooth
    fx->formant_shift = 0.5f;     // Neutral
    fx->release = 0.4f;           // Moderate release (robotic)
    fx->mix = 1.0f;               // 100% wet
    fx->carrier_mode = VOCODER_CARRIER_INTERNAL;
    fx->midi_note = 60;           // Middle C
    fx->carrier_phase = 0.0f;
    fx->sample_rate = 44100;
    fx->last_formant_shift = -1.0f;
    fx->last_release = -1.0f;
    fx->filters_initialized = false;

    return fx;
}

void fx_vocoder_destroy(FXVocoder* fx) {
    if (fx) free(fx);
}

void fx_vocoder_reset(FXVocoder* fx) {
    if (!fx) return;

    fx->carrier_phase = 0.0f;
    fx->filters_initialized = false;

    for (int i = 0; i < NUM_BANDS; i++) {
        fx->modulator_bands[i].x1 = fx->modulator_bands[i].x2 = 0.0f;
        fx->modulator_bands[i].y1 = fx->modulator_bands[i].y2 = 0.0f;

        fx->carrier_bands[i].x1 = fx->carrier_bands[i].x2 = 0.0f;
        fx->carrier_bands[i].y1 = fx->carrier_bands[i].y2 = 0.0f;

        fx->envelopes[i].level = 0.0f;
    }
}

// ============================================================================
// MIDI Note to Frequency Conversion
// ============================================================================

static float midi_note_to_freq(int note) {
    return 440.0f * powf(2.0f, ((float)note - 69.0f) / 12.0f);
}

// ============================================================================
// Processing Core
// ============================================================================

static void vocoder_process_core(FXVocoder* fx,
                                 const float* modulator_input,
                                 const float* carrier_input,
                                 float* output,
                                 int frames,
                                 int sample_rate,
                                 bool use_external_carrier) {
    if (!fx) return;

    // Update sample rate if changed
    bool sample_rate_changed = false;
    if (fx->sample_rate != sample_rate) {
        fx->sample_rate = sample_rate;
        sample_rate_changed = true;
        fx->filters_initialized = false;
    }

    // Re-init filters & envelopes if needed
    bool params_changed = (!fx->filters_initialized ||
                           fx->formant_shift != fx->last_formant_shift ||
                           fx->release       != fx->last_release       ||
                           sample_rate_changed);

    float shift_factor = powf(2.0f, (fx->formant_shift - 0.5f) * 2.0f); // ±1 octave

    if (params_changed) {
        for (int i = 0; i < NUM_BANDS; i++) {
            float freq = BAND_FREQUENCIES[i] * shift_factor;
            float nyquist = 0.5f * (float)sample_rate;
            if (freq < 80.0f) freq = 80.0f;
            if (freq > nyquist * 0.9f) freq = nyquist * 0.9f;

            float q = 2.0f; // medium width, simple

            biquad_bp_init(&fx->modulator_bands[i], freq, q, sample_rate);
            biquad_bp_init(&fx->carrier_bands[i],   freq, q, sample_rate);
            envelope_follower_init(&fx->envelopes[i], fx->release, sample_rate);
        }
        fx->last_formant_shift   = fx->formant_shift;
        fx->last_release         = fx->release;
        fx->filters_initialized  = true;
    }

    // Carrier frequency
    float carrier_freq;
    if (fx->carrier_mode == VOCODER_CARRIER_MIDI) {
        carrier_freq = midi_note_to_freq(fx->midi_note);
    } else {
        carrier_freq = MIN_CARRIER_FREQ + fx->carrier_freq * (MAX_CARRIER_FREQ - MIN_CARRIER_FREQ);
    }
    float phase_inc = 2.0f * M_PI * carrier_freq / (float)sample_rate;

    for (int i = 0; i < frames; i++) {
        float mod = modulator_input[i];
        float dry = mod;

        // Carrier sample
        float carrier;
        if (use_external_carrier && carrier_input) {
            carrier = carrier_input[i];
        } else {
            carrier = generate_carrier(fx->carrier_phase, fx->carrier_wave);
            fx->carrier_phase += phase_inc;
            if (fx->carrier_phase >= 2.0f * M_PI)
                fx->carrier_phase -= 2.0f * M_PI;
        }

        float wet = 0.0f;

        // Plain analysis/synthesis
        for (int b = 0; b < NUM_BANDS; b++) {
            float mod_band = biquad_bp_process(&fx->modulator_bands[b], mod);
            float env      = envelope_follower_process(&fx->envelopes[b], mod_band);
            float car_band = biquad_bp_process(&fx->carrier_bands[b], carrier);
            wet += car_band * env;
        }

        // Basic normalization by band count
        wet /= (float)NUM_BANDS;

        // No saturation, no sibilance, just mix
        output[i] = dry * (1.0f - fx->mix) + wet * fx->mix;
    }
}

// ============================================================================
// Processing wrappers
// ============================================================================

void fx_vocoder_process_f32(FXVocoder* fx, float* buffer, int frames, int sample_rate) {
    if (!fx || !buffer) return;
    if (!fx->enabled) return;

    // Allocate temp buffers for processing
    float* modulator = (float*)malloc((size_t)frames * sizeof(float));
    float* output    = (float*)malloc((size_t)frames * sizeof(float));
    if (!modulator || !output) {
        free(modulator);
        free(output);
        return;
    }

    // Sum stereo to mono for modulator
    for (int i = 0; i < frames; i++) {
        modulator[i] = 0.5f * (buffer[2 * i] + buffer[2 * i + 1]);
    }

    // Single-input mode: internal or MIDI carrier only
    bool use_external = false;
    vocoder_process_core(fx, modulator, NULL, output, frames, sample_rate, use_external);

    // Write mono output to both channels
    for (int i = 0; i < frames; i++) {
        buffer[2 * i]     = output[i];
        buffer[2 * i + 1] = output[i];
    }

    free(modulator);
    free(output);
}

void fx_vocoder_process_dual_f32(FXVocoder* fx,
                                 const float* modulator,
                                 const float* carrier,
                                 float* output,
                                 int frames,
                                 int sample_rate) {
    if (!fx || !modulator || !output) return;

    if (!fx->enabled) {
        // Passthrough modulator
        memcpy(output, modulator, (size_t)frames * sizeof(float));
        return;
    }

    bool use_external = (fx->carrier_mode == VOCODER_CARRIER_EXTERNAL && carrier != NULL);
    vocoder_process_core(fx, modulator, carrier, output, frames, sample_rate, use_external);
}

// ============================================================================
// Parameters
// ============================================================================

void fx_vocoder_set_enabled(FXVocoder* fx, bool enabled) {
    if (fx) fx->enabled = enabled;
}

bool fx_vocoder_get_enabled(FXVocoder* fx) {
    return fx ? fx->enabled : false;
}

void fx_vocoder_set_carrier_freq(FXVocoder* fx, fx_param_t freq) {
    if (fx) fx->carrier_freq = fmaxf(0.0f, fminf(1.0f, freq));
}

float fx_vocoder_get_carrier_freq(FXVocoder* fx) {
    return fx ? fx->carrier_freq : 0.3f;
}

void fx_vocoder_set_carrier_wave(FXVocoder* fx, fx_param_t wave) {
    if (fx) fx->carrier_wave = fmaxf(0.0f, fminf(1.0f, wave));
}

float fx_vocoder_get_carrier_wave(FXVocoder* fx) {
    return fx ? fx->carrier_wave : 0.0f;
}

void fx_vocoder_set_formant_shift(FXVocoder* fx, fx_param_t shift) {
    if (fx) {
        fx->formant_shift = fmaxf(0.0f, fminf(1.0f, shift));
        fx->filters_initialized = false;
    }
}

float fx_vocoder_get_formant_shift(FXVocoder* fx) {
    return fx ? fx->formant_shift : 0.5f;
}

void fx_vocoder_set_release(FXVocoder* fx, fx_param_t release) {
    if (fx) {
        fx->release = fmaxf(0.0f, fminf(1.0f, release));
        fx->filters_initialized = false;
    }
}

float fx_vocoder_get_release(FXVocoder* fx) {
    return fx ? fx->release : 0.4f;
}

void fx_vocoder_set_mix(FXVocoder* fx, fx_param_t mix) {
    if (fx) fx->mix = fmaxf(0.0f, fminf(1.0f, mix));
}

float fx_vocoder_get_mix(FXVocoder* fx) {
    return fx ? fx->mix : 1.0f;
}

void fx_vocoder_set_carrier_mode(FXVocoder* fx, fx_param_t mode_param) {
    if (!fx) return;

    if (mode_param < 0.34f) {
        fx->carrier_mode = VOCODER_CARRIER_INTERNAL;
    } else if (mode_param < 0.67f) {
        fx->carrier_mode = VOCODER_CARRIER_EXTERNAL;
    } else {
        fx->carrier_mode = VOCODER_CARRIER_MIDI;
    }
}

float fx_vocoder_get_carrier_mode(FXVocoder* fx) {
    if (!fx) return 0.0f;

    switch (fx->carrier_mode) {
        case VOCODER_CARRIER_INTERNAL: return 0.0f;
        case VOCODER_CARRIER_EXTERNAL: return 0.5f;
        case VOCODER_CARRIER_MIDI:     return 1.0f;
        default:                       return 0.0f;
    }
}

void fx_vocoder_set_midi_note(FXVocoder* fx, fx_param_t note_param) {
    if (!fx) return;
    int note = (int)(note_param * 127.0f + 0.5f);
    if (note < 0) note = 0;
    if (note > 127) note = 127;
    fx->midi_note = note;
}

float fx_vocoder_get_midi_note(FXVocoder* fx) {
    if (!fx) return 60.0f / 127.0f;
    return (float)fx->midi_note / 127.0f;
}

// ============================================================================
// Generic Parameter Interface
// ============================================================================

enum {
    PARAM_CARRIER_FREQ = 0,
    PARAM_CARRIER_WAVE,
    PARAM_FORMANT_SHIFT,
    PARAM_RELEASE,
    PARAM_MIX,
    PARAM_COUNT
};

int fx_vocoder_get_parameter_count(void) {
    return PARAM_COUNT;
}

float fx_vocoder_get_parameter_value(FXVocoder* fx, int index) {
    if (!fx) return 0.0f;

    switch (index) {
        case PARAM_CARRIER_FREQ:  return fx_vocoder_get_carrier_freq(fx);
        case PARAM_CARRIER_WAVE:  return fx_vocoder_get_carrier_wave(fx);
        case PARAM_FORMANT_SHIFT: return fx_vocoder_get_formant_shift(fx);
        case PARAM_RELEASE:       return fx_vocoder_get_release(fx);
        case PARAM_MIX:           return fx_vocoder_get_mix(fx);
        default:                  return 0.0f;
    }
}

void fx_vocoder_set_parameter_value(FXVocoder* fx, int index, float value) {
    if (!fx) return;

    switch (index) {
        case PARAM_CARRIER_FREQ:  fx_vocoder_set_carrier_freq(fx, value);  break;
        case PARAM_CARRIER_WAVE:  fx_vocoder_set_carrier_wave(fx, value);  break;
        case PARAM_FORMANT_SHIFT: fx_vocoder_set_formant_shift(fx, value); break;
        case PARAM_RELEASE:       fx_vocoder_set_release(fx, value);       break;
        case PARAM_MIX:           fx_vocoder_set_mix(fx, value);           break;
    }
}

const char* fx_vocoder_get_parameter_name(int index) {
    switch (index) {
        case PARAM_CARRIER_FREQ:  return "Carrier Freq";
        case PARAM_CARRIER_WAVE:  return "Carrier Wave";
        case PARAM_FORMANT_SHIFT: return "Formant Shift";
        case PARAM_RELEASE:       return "Release";
        case PARAM_MIX:           return "Mix";
        default:                  return "";
    }
}

const char* fx_vocoder_get_parameter_label(int index) {
    switch (index) {
        case PARAM_CARRIER_FREQ:  return "Hz";
        case PARAM_CARRIER_WAVE:  return "";
        case PARAM_FORMANT_SHIFT: return "";
        case PARAM_RELEASE:       return "ms";
        case PARAM_MIX:           return "%";
        default:                  return "";
    }
}

float fx_vocoder_get_parameter_default(int index) {
    switch (index) {
        case PARAM_CARRIER_FREQ:  return 0.3f;  // ~200 Hz
        case PARAM_CARRIER_WAVE:  return 0.0f;  // Saw
        case PARAM_FORMANT_SHIFT: return 0.5f;  // Neutral
        case PARAM_RELEASE:       return 0.4f;  // Robotic-ish
        case PARAM_MIX:           return 1.0f;  // 100% wet
        default:                  return 0.0f;
    }
}

float fx_vocoder_get_parameter_min(int index) {
    (void)index;
    return 0.0f;
}

float fx_vocoder_get_parameter_max(int index) {
    (void)index;
    return 1.0f;
}

int fx_vocoder_parameter_is_boolean(int index) {
    (void)index;
    return 0;
}

int fx_vocoder_parameter_is_integer(int index) {
    (void)index;
    return 0;
}
