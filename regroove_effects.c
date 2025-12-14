/*
 * Regroove Effects - Legacy Wrapper API
 *
 * This file provides backward compatibility by wrapping the individual
 * effect modules into a single aggregated interface.
 *
 * New code should use the individual modules directly (see effects/)
 */

#include "regroove_effects.h"
#include "effects/fx_distortion.h"
#include "effects/fx_filter.h"
#include "effects/fx_eq.h"
#include "effects/fx_compressor.h"
#include "effects/fx_delay.h"

#include <stdlib.h>
#include <string.h>

// Internal structure - wraps individual effect modules
struct RegrooveEffects {
    // Individual effect modules
    FXDistortion* distortion;
    FXFilter* filter;
    FXEqualizer* eq;
    FXCompressor* compressor;
    FXDelay* delay;

    // Temporary buffer for processing (stereo interleaved)
    float* temp_buffer;
    int temp_buffer_size;
};

RegrooveEffects* regroove_effects_create(void) {
    RegrooveEffects* fx = (RegrooveEffects*)calloc(1, sizeof(RegrooveEffects));
    if (!fx) return NULL;

    // Create individual effect modules
    fx->distortion = fx_distortion_create();
    fx->filter = fx_filter_create();
    fx->eq = fx_eq_create();
    fx->compressor = fx_compressor_create();
    fx->delay = fx_delay_create();

    if (!fx->distortion || !fx->filter || !fx->eq || !fx->compressor || !fx->delay) {
        regroove_effects_destroy(fx);
        return NULL;
    }

    // Allocate temporary processing buffer (will resize as needed)
    fx->temp_buffer_size = 0;
    fx->temp_buffer = NULL;

    return fx;
}

void regroove_effects_destroy(RegrooveEffects* fx) {
    if (!fx) return;

    if (fx->distortion) fx_distortion_destroy(fx->distortion);
    if (fx->filter) fx_filter_destroy(fx->filter);
    if (fx->eq) fx_eq_destroy(fx->eq);
    if (fx->compressor) fx_compressor_destroy(fx->compressor);
    if (fx->delay) fx_delay_destroy(fx->delay);

    if (fx->temp_buffer) free(fx->temp_buffer);

    free(fx);
}

void regroove_effects_reset(RegrooveEffects* fx) {
    if (!fx) return;

    if (fx->distortion) fx_distortion_reset(fx->distortion);
    if (fx->filter) fx_filter_reset(fx->filter);
    if (fx->eq) fx_eq_reset(fx->eq);
    if (fx->compressor) fx_compressor_reset(fx->compressor);
    if (fx->delay) fx_delay_reset(fx->delay);
}

// Process audio through the effects chain
void regroove_effects_process(RegrooveEffects* fx, int16_t* buffer, int frames, int sample_rate) {
    if (!fx || !buffer || frames <= 0) return;

    // Ensure temp buffer is large enough (interleaved stereo)
    int needed_size = frames * 2;
    if (fx->temp_buffer_size < needed_size) {
        float* new_buffer = (float*)realloc(fx->temp_buffer, needed_size * sizeof(float));
        if (!new_buffer) return;
        fx->temp_buffer = new_buffer;
        fx->temp_buffer_size = needed_size;
    }

    // Convert int16 to float
    for (int i = 0; i < frames * 2; i++) {
        fx->temp_buffer[i] = buffer[i] / 32768.0f;
    }

    // Process through effect chain (in order)
    if (fx->distortion) {
        fx_distortion_process_f32(fx->distortion, fx->temp_buffer, frames, sample_rate);
    }

    if (fx->filter) {
        fx_filter_process_f32(fx->filter, fx->temp_buffer, frames, sample_rate);
    }

    if (fx->eq) {
        fx_eq_process_f32(fx->eq, fx->temp_buffer, frames, sample_rate);
    }

    if (fx->compressor) {
        fx_compressor_process_f32(fx->compressor, fx->temp_buffer, frames, sample_rate);
    }

    if (fx->delay) {
        fx_delay_process_f32(fx->delay, fx->temp_buffer, frames, sample_rate);
    }

    // Convert float back to int16
    for (int i = 0; i < frames * 2; i++) {
        float sample = fx->temp_buffer[i];
        // Clamp to prevent overflow
        if (sample > 1.0f) sample = 1.0f;
        if (sample < -1.0f) sample = -1.0f;
        buffer[i] = (int16_t)(sample * 32767.0f);
    }
}

// Process audio through the effects chain (float32 version - preferred for plugins)
void regroove_effects_process_f32(RegrooveEffects* fx, float* buffer, int frames, int sample_rate) {
    if (!fx || !buffer || frames <= 0) return;

    // Process directly on float buffer - no conversion needed!
    // Process through effect chain (in order)
    if (fx->distortion) {
        fx_distortion_process_f32(fx->distortion, buffer, frames, sample_rate);
    }

    if (fx->filter) {
        fx_filter_process_f32(fx->filter, buffer, frames, sample_rate);
    }

    if (fx->eq) {
        fx_eq_process_f32(fx->eq, buffer, frames, sample_rate);
    }

    if (fx->compressor) {
        fx_compressor_process_f32(fx->compressor, buffer, frames, sample_rate);
    }

    if (fx->delay) {
        fx_delay_process_f32(fx->delay, buffer, frames, sample_rate);
    }

    // Note: No clamping needed here - individual effects handle their own gain staging
}

// Distortion parameters
void regroove_effects_set_distortion_enabled(RegrooveEffects* fx, int enabled) {
    if (fx && fx->distortion) fx_distortion_set_enabled(fx->distortion, enabled);
}

void regroove_effects_set_distortion_drive(RegrooveEffects* fx, float drive) {
    if (fx && fx->distortion) fx_distortion_set_drive(fx->distortion, drive);
}

void regroove_effects_set_distortion_mix(RegrooveEffects* fx, float mix) {
    if (fx && fx->distortion) fx_distortion_set_mix(fx->distortion, mix);
}

int regroove_effects_get_distortion_enabled(RegrooveEffects* fx) {
    return (fx && fx->distortion) ? fx_distortion_get_enabled(fx->distortion) : 0;
}

float regroove_effects_get_distortion_drive(RegrooveEffects* fx) {
    return (fx && fx->distortion) ? fx_distortion_get_drive(fx->distortion) : 0.0f;
}

float regroove_effects_get_distortion_mix(RegrooveEffects* fx) {
    return (fx && fx->distortion) ? fx_distortion_get_mix(fx->distortion) : 0.0f;
}

// Filter parameters
void regroove_effects_set_filter_enabled(RegrooveEffects* fx, int enabled) {
    if (fx && fx->filter) fx_filter_set_enabled(fx->filter, enabled);
}

void regroove_effects_set_filter_cutoff(RegrooveEffects* fx, float cutoff) {
    if (fx && fx->filter) fx_filter_set_cutoff(fx->filter, cutoff);
}

void regroove_effects_set_filter_resonance(RegrooveEffects* fx, float resonance) {
    if (fx && fx->filter) fx_filter_set_resonance(fx->filter, resonance);
}

int regroove_effects_get_filter_enabled(RegrooveEffects* fx) {
    return (fx && fx->filter) ? fx_filter_get_enabled(fx->filter) : 0;
}

float regroove_effects_get_filter_cutoff(RegrooveEffects* fx) {
    return (fx && fx->filter) ? fx_filter_get_cutoff(fx->filter) : 0.0f;
}

float regroove_effects_get_filter_resonance(RegrooveEffects* fx) {
    return (fx && fx->filter) ? fx_filter_get_resonance(fx->filter) : 0.0f;
}

// Delay parameters
void regroove_effects_set_delay_enabled(RegrooveEffects* fx, int enabled) {
    if (fx && fx->delay) fx_delay_set_enabled(fx->delay, enabled);
}

void regroove_effects_set_delay_time(RegrooveEffects* fx, float time) {
    if (fx && fx->delay) fx_delay_set_time(fx->delay, time);
}

void regroove_effects_set_delay_feedback(RegrooveEffects* fx, float feedback) {
    if (fx && fx->delay) fx_delay_set_feedback(fx->delay, feedback);
}

void regroove_effects_set_delay_mix(RegrooveEffects* fx, float mix) {
    if (fx && fx->delay) fx_delay_set_mix(fx->delay, mix);
}

int regroove_effects_get_delay_enabled(RegrooveEffects* fx) {
    return (fx && fx->delay) ? fx_delay_get_enabled(fx->delay) : 0;
}

float regroove_effects_get_delay_time(RegrooveEffects* fx) {
    return (fx && fx->delay) ? fx_delay_get_time(fx->delay) : 0.0f;
}

float regroove_effects_get_delay_feedback(RegrooveEffects* fx) {
    return (fx && fx->delay) ? fx_delay_get_feedback(fx->delay) : 0.0f;
}

float regroove_effects_get_delay_mix(RegrooveEffects* fx) {
    return (fx && fx->delay) ? fx_delay_get_mix(fx->delay) : 0.0f;
}

// EQ parameters
void regroove_effects_set_eq_enabled(RegrooveEffects* fx, int enabled) {
    if (fx && fx->eq) fx_eq_set_enabled(fx->eq, enabled);
}

void regroove_effects_set_eq_low(RegrooveEffects* fx, float gain) {
    if (fx && fx->eq) fx_eq_set_low(fx->eq, gain);
}

void regroove_effects_set_eq_mid(RegrooveEffects* fx, float gain) {
    if (fx && fx->eq) fx_eq_set_mid(fx->eq, gain);
}

void regroove_effects_set_eq_high(RegrooveEffects* fx, float gain) {
    if (fx && fx->eq) fx_eq_set_high(fx->eq, gain);
}

int regroove_effects_get_eq_enabled(RegrooveEffects* fx) {
    return (fx && fx->eq) ? fx_eq_get_enabled(fx->eq) : 0;
}

float regroove_effects_get_eq_low(RegrooveEffects* fx) {
    return (fx && fx->eq) ? fx_eq_get_low(fx->eq) : 0.5f;
}

float regroove_effects_get_eq_mid(RegrooveEffects* fx) {
    return (fx && fx->eq) ? fx_eq_get_mid(fx->eq) : 0.5f;
}

float regroove_effects_get_eq_high(RegrooveEffects* fx) {
    return (fx && fx->eq) ? fx_eq_get_high(fx->eq) : 0.5f;
}

// Compressor parameters
void regroove_effects_set_compressor_enabled(RegrooveEffects* fx, int enabled) {
    if (fx && fx->compressor) fx_compressor_set_enabled(fx->compressor, enabled);
}

void regroove_effects_set_compressor_threshold(RegrooveEffects* fx, float threshold) {
    if (fx && fx->compressor) fx_compressor_set_threshold(fx->compressor, threshold);
}

void regroove_effects_set_compressor_ratio(RegrooveEffects* fx, float ratio) {
    if (fx && fx->compressor) fx_compressor_set_ratio(fx->compressor, ratio);
}

void regroove_effects_set_compressor_attack(RegrooveEffects* fx, float attack) {
    if (fx && fx->compressor) fx_compressor_set_attack(fx->compressor, attack);
}

void regroove_effects_set_compressor_release(RegrooveEffects* fx, float release) {
    if (fx && fx->compressor) fx_compressor_set_release(fx->compressor, release);
}

void regroove_effects_set_compressor_makeup(RegrooveEffects* fx, float makeup) {
    if (fx && fx->compressor) fx_compressor_set_makeup(fx->compressor, makeup);
}

int regroove_effects_get_compressor_enabled(RegrooveEffects* fx) {
    return (fx && fx->compressor) ? fx_compressor_get_enabled(fx->compressor) : 0;
}

float regroove_effects_get_compressor_threshold(RegrooveEffects* fx) {
    return (fx && fx->compressor) ? fx_compressor_get_threshold(fx->compressor) : 0.5f;
}

float regroove_effects_get_compressor_ratio(RegrooveEffects* fx) {
    return (fx && fx->compressor) ? fx_compressor_get_ratio(fx->compressor) : 0.5f;
}

float regroove_effects_get_compressor_attack(RegrooveEffects* fx) {
    return (fx && fx->compressor) ? fx_compressor_get_attack(fx->compressor) : 0.5f;
}

float regroove_effects_get_compressor_release(RegrooveEffects* fx) {
    return (fx && fx->compressor) ? fx_compressor_get_release(fx->compressor) : 0.5f;
}

float regroove_effects_get_compressor_makeup(RegrooveEffects* fx) {
    return (fx && fx->compressor) ? fx_compressor_get_makeup(fx->compressor) : 0.5f;
}

