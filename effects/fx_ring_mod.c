/*
 * Ring Modulator Effect Implementation
 */

#include "fx_ring_mod.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

struct FXRingMod {
    int enabled;
    float frequency;    // 0.0 - 1.0 (normalized)
    float mix;          // 0.0 - 1.0

    // Internal carrier oscillator
    float carrier_phase;  // 0.0 - 2*PI
};

// ============================================================================
// Lifecycle
// ============================================================================

FXRingMod* fx_ring_mod_create(void) {
    FXRingMod* fx = (FXRingMod*)malloc(sizeof(FXRingMod));
    if (!fx) return NULL;

    memset(fx, 0, sizeof(FXRingMod));

    fx->enabled = 0;
    fx->frequency = 0.1f;  // ~500 Hz default
    fx->mix = 1.0f;        // 100% wet default
    fx->carrier_phase = 0.0f;

    return fx;
}

void fx_ring_mod_destroy(FXRingMod* fx) {
    if (fx) {
        free(fx);
    }
}

void fx_ring_mod_reset(FXRingMod* fx) {
    if (!fx) return;
    fx->carrier_phase = 0.0f;
}

// ============================================================================
// Processing
// ============================================================================

void fx_ring_mod_process_f32(FXRingMod* fx, float* buffer, int frames, int sample_rate) {
    if (!fx || !buffer) return;
    if (!fx->enabled) return;

    // Map normalized frequency (0-1) to Hz (20 - 5000 Hz)
    float carrier_freq = 20.0f + fx->frequency * 4980.0f;
    float phase_increment = 2.0f * M_PI * carrier_freq / sample_rate;

    for (int i = 0; i < frames; i++) {
        // Generate carrier oscillator (sine wave)
        float carrier = sinf(fx->carrier_phase);

        // Advance carrier phase
        fx->carrier_phase += phase_increment;
        if (fx->carrier_phase >= 2.0f * M_PI) {
            fx->carrier_phase -= 2.0f * M_PI;
        }

        // Ring modulation: multiply input by carrier
        float dry_l = buffer[i * 2];
        float dry_r = buffer[i * 2 + 1];

        float wet_l = dry_l * carrier;
        float wet_r = dry_r * carrier;

        // Mix dry/wet
        buffer[i * 2] = dry_l * (1.0f - fx->mix) + wet_l * fx->mix;
        buffer[i * 2 + 1] = dry_r * (1.0f - fx->mix) + wet_r * fx->mix;
    }
}

// ============================================================================
// Parameters
// ============================================================================

void fx_ring_mod_set_enabled(FXRingMod* fx, int enabled) {
    if (!fx) return;
    fx->enabled = enabled;
}

int fx_ring_mod_get_enabled(FXRingMod* fx) {
    if (!fx) return 0;
    return fx->enabled;
}

void fx_ring_mod_set_frequency(FXRingMod* fx, float frequency) {
    if (!fx) return;
    if (frequency < 0.0f) frequency = 0.0f;
    if (frequency > 1.0f) frequency = 1.0f;
    fx->frequency = frequency;
}

float fx_ring_mod_get_frequency(FXRingMod* fx) {
    if (!fx) return 0.1f;
    return fx->frequency;
}

void fx_ring_mod_set_mix(FXRingMod* fx, float mix) {
    if (!fx) return;
    if (mix < 0.0f) mix = 0.0f;
    if (mix > 1.0f) mix = 1.0f;
    fx->mix = mix;
}

float fx_ring_mod_get_mix(FXRingMod* fx) {
    if (!fx) return 1.0f;
    return fx->mix;
}

// ============================================================================
// Generic Parameter Interface
// ============================================================================

enum {
    PARAM_FREQUENCY = 0,
    PARAM_MIX,
    PARAM_COUNT
};

int fx_ring_mod_get_parameter_count(void) {
    return PARAM_COUNT;
}

float fx_ring_mod_get_parameter(FXRingMod* fx, int index) {
    if (!fx) return 0.0f;

    switch (index) {
        case PARAM_FREQUENCY: return fx->frequency;
        case PARAM_MIX: return fx->mix;
        default: return 0.0f;
    }
}

void fx_ring_mod_set_parameter(FXRingMod* fx, int index, float value) {
    if (!fx) return;

    switch (index) {
        case PARAM_FREQUENCY: fx_ring_mod_set_frequency(fx, value); break;
        case PARAM_MIX: fx_ring_mod_set_mix(fx, value); break;
    }
}

const char* fx_ring_mod_get_parameter_name(int index) {
    switch (index) {
        case PARAM_FREQUENCY: return "Frequency";
        case PARAM_MIX: return "Mix";
        default: return "";
    }
}

const char* fx_ring_mod_get_parameter_label(int index) {
    switch (index) {
        case PARAM_FREQUENCY: return "Hz";
        case PARAM_MIX: return "%";
        default: return "";
    }
}

float fx_ring_mod_get_parameter_default(int index) {
    switch (index) {
        case PARAM_FREQUENCY: return 0.1f;  // ~500 Hz
        case PARAM_MIX: return 1.0f;        // 100% wet
        default: return 0.0f;
    }
}

float fx_ring_mod_get_parameter_min(int index) {
    (void)index;
    return 0.0f;
}

float fx_ring_mod_get_parameter_max(int index) {
    (void)index;
    return 1.0f;
}

int fx_ring_mod_parameter_is_boolean(int index) {
    (void)index;
    return 0;
}

int fx_ring_mod_parameter_is_integer(int index) {
    (void)index;
    return 0;
}
