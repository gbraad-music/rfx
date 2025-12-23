/*
 * Ring Modulator Effect
 * Classic analog ring modulation with internal carrier oscillator
 *
 * Ring modulation multiplies the input signal with a carrier oscillator,
 * creating sum and difference frequencies (f1 + f2, f1 - f2).
 * No original signal remains in the output.
 *
 * Copyright (C) 2024
 * SPDX-License-Identifier: ISC
 */

#ifndef FX_RING_MOD_H
#define FX_RING_MOD_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct FXRingMod FXRingMod;

// ============================================================================
// Lifecycle
// ============================================================================

FXRingMod* fx_ring_mod_create(void);
void fx_ring_mod_destroy(FXRingMod* fx);
void fx_ring_mod_reset(FXRingMod* fx);

// ============================================================================
// Processing
// ============================================================================

/**
 * Process stereo audio buffer (interleaved float32)
 * @param buffer Interleaved stereo buffer (L, R, L, R, ...)
 * @param frames Number of stereo frames
 * @param sample_rate Sample rate in Hz
 */
void fx_ring_mod_process_f32(FXRingMod* fx, float* buffer, int frames, int sample_rate);

// ============================================================================
// Parameters
// ============================================================================

void fx_ring_mod_set_enabled(FXRingMod* fx, int enabled);
int fx_ring_mod_get_enabled(FXRingMod* fx);

/**
 * Set carrier frequency (0.0 - 1.0)
 * Maps to: 20 Hz - 5000 Hz (musical range for ring mod)
 */
void fx_ring_mod_set_frequency(FXRingMod* fx, float frequency);
float fx_ring_mod_get_frequency(FXRingMod* fx);

/**
 * Set dry/wet mix (0.0 = dry, 1.0 = wet)
 */
void fx_ring_mod_set_mix(FXRingMod* fx, float mix);
float fx_ring_mod_get_mix(FXRingMod* fx);

// ============================================================================
// Generic Parameter Interface
// ============================================================================

int fx_ring_mod_get_parameter_count(void);
float fx_ring_mod_get_parameter(FXRingMod* fx, int index);
void fx_ring_mod_set_parameter(FXRingMod* fx, int index, float value);
const char* fx_ring_mod_get_parameter_name(int index);
const char* fx_ring_mod_get_parameter_label(int index);
float fx_ring_mod_get_parameter_default(int index);
float fx_ring_mod_get_parameter_min(int index);
float fx_ring_mod_get_parameter_max(int index);
int fx_ring_mod_parameter_is_boolean(int index);
int fx_ring_mod_parameter_is_integer(int index);

#ifdef __cplusplus
}
#endif

#endif // FX_RING_MOD_H
