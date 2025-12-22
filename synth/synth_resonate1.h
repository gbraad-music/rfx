/*
 * RGResonate1 - Polyphonic Subtractive Synthesizer
 *
 * Classic analog-style subtractive synthesis with:
 * - Polyphonic voice allocation (8 voices)
 * - Selectable waveforms (saw, square, triangle, sine)
 * - Moog ladder filter with resonance
 * - ADSR envelope generator
 * - Velocity sensitivity
 *
 * Copyright (C) 2024
 * SPDX-License-Identifier: ISC
 */

#ifndef SYNTH_RESONATE1_H
#define SYNTH_RESONATE1_H

#include "synth_common.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct SynthResonate1 SynthResonate1;

// Waveform types
typedef enum {
    RESONATE1_WAVE_SAW = 0,
    RESONATE1_WAVE_SQUARE,
    RESONATE1_WAVE_TRIANGLE,
    RESONATE1_WAVE_SINE
} Resonate1Waveform;

// ============================================================================
// Lifecycle
// ============================================================================

/**
 * Create a new RGResonate1 synthesizer instance
 * @param sample_rate Sample rate in Hz
 * @return New synthesizer instance or NULL on failure
 */
SynthResonate1* synth_resonate1_create(int sample_rate);

/**
 * Destroy a synthesizer instance
 */
void synth_resonate1_destroy(SynthResonate1* synth);

/**
 * Reset synthesizer state (all notes off, clear buffers)
 */
void synth_resonate1_reset(SynthResonate1* synth);

// ============================================================================
// MIDI Event Handling
// ============================================================================

/**
 * Trigger a note on event
 * @param note MIDI note number (0-127)
 * @param velocity MIDI velocity (0-127)
 */
void synth_resonate1_note_on(SynthResonate1* synth, uint8_t note, uint8_t velocity);

/**
 * Trigger a note off event
 * @param note MIDI note number (0-127)
 */
void synth_resonate1_note_off(SynthResonate1* synth, uint8_t note);

/**
 * All notes off (panic button)
 */
void synth_resonate1_all_notes_off(SynthResonate1* synth);

// ============================================================================
// Audio Processing
// ============================================================================

/**
 * Process audio buffer (stereo interleaved float32)
 * @param buffer Interleaved stereo buffer (L, R, L, R, ...)
 * @param frames Number of stereo frames
 * @param sample_rate Sample rate in Hz
 */
void synth_resonate1_process_f32(SynthResonate1* synth, float* buffer,
                                  int frames, int sample_rate);

// ============================================================================
// Parameters (normalized 0.0-1.0 for automation)
// ============================================================================

// Oscillator
void synth_resonate1_set_waveform(SynthResonate1* synth, Resonate1Waveform waveform);
Resonate1Waveform synth_resonate1_get_waveform(SynthResonate1* synth);

// Filter
void synth_resonate1_set_filter_cutoff(SynthResonate1* synth, float cutoff);      // 0.0-1.0
void synth_resonate1_set_filter_resonance(SynthResonate1* synth, float resonance); // 0.0-1.0
float synth_resonate1_get_filter_cutoff(SynthResonate1* synth);
float synth_resonate1_get_filter_resonance(SynthResonate1* synth);

// Amplitude Envelope (ADSR)
void synth_resonate1_set_amp_attack(SynthResonate1* synth, float attack);     // 0.0-1.0 (maps to 0.001-2.0 seconds)
void synth_resonate1_set_amp_decay(SynthResonate1* synth, float decay);       // 0.0-1.0 (maps to 0.001-2.0 seconds)
void synth_resonate1_set_amp_sustain(SynthResonate1* synth, float sustain);   // 0.0-1.0 (level)
void synth_resonate1_set_amp_release(SynthResonate1* synth, float release);   // 0.0-1.0 (maps to 0.001-4.0 seconds)

float synth_resonate1_get_amp_attack(SynthResonate1* synth);
float synth_resonate1_get_amp_decay(SynthResonate1* synth);
float synth_resonate1_get_amp_sustain(SynthResonate1* synth);
float synth_resonate1_get_amp_release(SynthResonate1* synth);

// Filter Envelope (ADSR)
void synth_resonate1_set_filter_env_amount(SynthResonate1* synth, float amount);  // 0.0-1.0
void synth_resonate1_set_filter_attack(SynthResonate1* synth, float attack);      // 0.0-1.0
void synth_resonate1_set_filter_decay(SynthResonate1* synth, float decay);        // 0.0-1.0
void synth_resonate1_set_filter_sustain(SynthResonate1* synth, float sustain);    // 0.0-1.0
void synth_resonate1_set_filter_release(SynthResonate1* synth, float release);    // 0.0-1.0

float synth_resonate1_get_filter_env_amount(SynthResonate1* synth);
float synth_resonate1_get_filter_attack(SynthResonate1* synth);
float synth_resonate1_get_filter_decay(SynthResonate1* synth);
float synth_resonate1_get_filter_sustain(SynthResonate1* synth);
float synth_resonate1_get_filter_release(SynthResonate1* synth);

// Voice Management
void synth_resonate1_set_polyphony(SynthResonate1* synth, int voices);  // 1-16
int synth_resonate1_get_polyphony(SynthResonate1* synth);

// ============================================================================
// Generic Parameter Interface (for wrapper use)
// ============================================================================

/**
 * Get total number of parameters
 */
int synth_resonate1_get_parameter_count(void);

/**
 * Get parameter value by index (normalized 0.0-1.0)
 */
float synth_resonate1_get_parameter_value(SynthResonate1* synth, int index);

/**
 * Set parameter value by index (normalized 0.0-1.0)
 */
void synth_resonate1_set_parameter_value(SynthResonate1* synth, int index, float value);

/**
 * Get parameter name
 */
const char* synth_resonate1_get_parameter_name(int index);

/**
 * Get parameter label/unit
 */
const char* synth_resonate1_get_parameter_label(int index);

/**
 * Get parameter default value (normalized 0.0-1.0)
 */
float synth_resonate1_get_parameter_default(int index);

/**
 * Get parameter minimum value (actual units)
 */
float synth_resonate1_get_parameter_min(int index);

/**
 * Get parameter maximum value (actual units)
 */
float synth_resonate1_get_parameter_max(int index);

/**
 * Get parameter group/category
 */
int synth_resonate1_get_parameter_group(int index);

/**
 * Get group name
 */
const char* synth_resonate1_get_group_name(int group);

/**
 * Check if parameter is integer
 */
int synth_resonate1_parameter_is_integer(int index);

#ifdef __cplusplus
}
#endif

#endif // SYNTH_RESONATE1_H
