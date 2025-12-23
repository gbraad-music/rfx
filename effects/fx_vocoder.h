/*
 * Vocoder Effect
 * Phase 1: Simple vocoder with internal carrier
 * Phase 2: Dual-input with external carrier and MIDI control
 *
 * Copyright (C) 2024
 * SPDX-License-Identifier: ISC
 */

#ifndef FX_VOCODER_H
#define FX_VOCODER_H

#include <stdbool.h>
#include "fx_common.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct FXVocoder FXVocoder;

// Carrier modes (for future extension)
typedef enum {
    VOCODER_CARRIER_INTERNAL = 0,   // Internal oscillator (default)
    VOCODER_CARRIER_EXTERNAL = 1,   // External audio input (Phase 2)
    VOCODER_CARRIER_MIDI = 2        // MIDI-controlled oscillator (Phase 2)
} VocoderCarrierMode;

// ============================================================================
// Lifecycle
// ============================================================================

FXVocoder* fx_vocoder_create(void);
void fx_vocoder_destroy(FXVocoder* fx);
void fx_vocoder_reset(FXVocoder* fx);

// ============================================================================
// Processing
// ============================================================================

// Simple processing (Phase 1: internal carrier)
void fx_vocoder_process_f32(FXVocoder* fx, float* buffer, int frames, int sample_rate);

// Dual-input processing (Phase 2: external carrier)
void fx_vocoder_process_dual_f32(FXVocoder* fx,
                                  const float* modulator,
                                  const float* carrier,
                                  float* output,
                                  int frames,
                                  int sample_rate);

// ============================================================================
// Parameters (0.0-1.0 normalized)
// ============================================================================

void fx_vocoder_set_enabled(FXVocoder* fx, bool enabled);
bool fx_vocoder_get_enabled(FXVocoder* fx);

// Carrier frequency (0.0-1.0 maps to 50-500 Hz)
void fx_vocoder_set_carrier_freq(FXVocoder* fx, fx_param_t freq);
float fx_vocoder_get_carrier_freq(FXVocoder* fx);

// Carrier waveform (0.0=sawtooth, 0.5=square, 1.0=pulse)
void fx_vocoder_set_carrier_wave(FXVocoder* fx, fx_param_t wave);
float fx_vocoder_get_carrier_wave(FXVocoder* fx);

// Formant shift (0.0-1.0, 0.5=neutral, <0.5=down, >0.5=up)
void fx_vocoder_set_formant_shift(FXVocoder* fx, fx_param_t shift);
float fx_vocoder_get_formant_shift(FXVocoder* fx);

// Envelope release time (0.0-1.0, 0=fast, 1=slow)
void fx_vocoder_set_release(FXVocoder* fx, fx_param_t release);
float fx_vocoder_get_release(FXVocoder* fx);

// Dry/wet mix
void fx_vocoder_set_mix(FXVocoder* fx, fx_param_t mix);
float fx_vocoder_get_mix(FXVocoder* fx);

// Carrier mode (Phase 2)
// 0.0-0.33 = Internal, 0.34-0.66 = External, 0.67-1.0 = MIDI
void fx_vocoder_set_carrier_mode(FXVocoder* fx, fx_param_t mode);
float fx_vocoder_get_carrier_mode(FXVocoder* fx);

// MIDI note (Phase 2 - for MIDI carrier mode)
// 0.0-1.0 maps to MIDI notes 0-127
void fx_vocoder_set_midi_note(FXVocoder* fx, fx_param_t note);
float fx_vocoder_get_midi_note(FXVocoder* fx);

// ============================================================================
// Generic Parameter Interface
// ============================================================================

int fx_vocoder_get_parameter_count(void);
float fx_vocoder_get_parameter_value(FXVocoder* fx, int index);
void fx_vocoder_set_parameter_value(FXVocoder* fx, int index, float value);
const char* fx_vocoder_get_parameter_name(int index);
const char* fx_vocoder_get_parameter_label(int index);
float fx_vocoder_get_parameter_default(int index);
float fx_vocoder_get_parameter_min(int index);
float fx_vocoder_get_parameter_max(int index);
int fx_vocoder_parameter_is_boolean(int index);
int fx_vocoder_parameter_is_integer(int index);

#ifdef __cplusplus
}
#endif

#endif // FX_VOCODER_H
