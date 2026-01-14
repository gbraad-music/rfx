/*
 * SID (MOS 6581/8580) Synthesizer Emulation
 * Classic Commodore 64 sound chip
 *
 * Features:
 * - 3 independent voices with waveforms (triangle, sawtooth, pulse, noise)
 * - ADSR envelope per voice
 * - Pulse width modulation
 * - Ring modulation and hard sync
 * - Multimode filter (LP/BP/HP) with resonance
 *
 * Copyright (C) 2024
 * SPDX-License-Identifier: ISC
 */

#ifndef SYNTH_SID_H
#define SYNTH_SID_H

#include "synth_common.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct SynthSID SynthSID;

// SID waveforms (can be combined, except noise)
typedef enum {
    SID_WAVE_NONE = 0,
    SID_WAVE_TRIANGLE = 1,
    SID_WAVE_SAWTOOTH = 2,
    SID_WAVE_PULSE = 4,
    SID_WAVE_NOISE = 8
} SIDWaveform;

// Filter modes
typedef enum {
    SID_FILTER_OFF = 0,
    SID_FILTER_LP,  // Low pass
    SID_FILTER_BP,  // Band pass
    SID_FILTER_HP   // High pass
} SIDFilterMode;

// ============================================================================
// Lifecycle
// ============================================================================

/**
 * Create a new SID synthesizer instance
 * @param sample_rate Sample rate in Hz
 * @return New synthesizer instance or NULL on failure
 */
SynthSID* synth_sid_create(int sample_rate);

/**
 * Destroy a synthesizer instance
 */
void synth_sid_destroy(SynthSID* sid);

/**
 * Reset synthesizer state (all voices off)
 */
void synth_sid_reset(SynthSID* sid);

// ============================================================================
// MIDI Event Handling
// ============================================================================

/**
 * Trigger a note on event
 * @param voice Voice number (0-2)
 * @param note MIDI note number (0-127)
 * @param velocity MIDI velocity (0-127)
 */
void synth_sid_note_on(SynthSID* sid, uint8_t voice, uint8_t note, uint8_t velocity);

/**
 * Trigger a note off event
 * @param voice Voice number (0-2)
 */
void synth_sid_note_off(SynthSID* sid, uint8_t voice);

/**
 * All notes off (panic button)
 */
void synth_sid_all_notes_off(SynthSID* sid);

// ============================================================================
// Audio Processing
// ============================================================================

/**
 * Process audio buffer (stereo interleaved float32)
 * @param buffer Interleaved stereo buffer (L, R, L, R, ...)
 * @param frames Number of stereo frames
 * @param sample_rate Sample rate in Hz
 */
void synth_sid_process_f32(SynthSID* sid, float* buffer, int frames, int sample_rate);

// ============================================================================
// Voice Parameters (0.0-1.0 normalized)
// ============================================================================

// Waveform selection
void synth_sid_set_waveform(SynthSID* sid, uint8_t voice, uint8_t waveform);
uint8_t synth_sid_get_waveform(SynthSID* sid, uint8_t voice);

// Pulse width (0.0-1.0, only affects pulse waveform)
void synth_sid_set_pulse_width(SynthSID* sid, uint8_t voice, float width);
float synth_sid_get_pulse_width(SynthSID* sid, uint8_t voice);

// ADSR envelope
void synth_sid_set_attack(SynthSID* sid, uint8_t voice, float attack);
void synth_sid_set_decay(SynthSID* sid, uint8_t voice, float decay);
void synth_sid_set_sustain(SynthSID* sid, uint8_t voice, float sustain);
void synth_sid_set_release(SynthSID* sid, uint8_t voice, float release);

// Ring modulation (voice modulates next voice)
void synth_sid_set_ring_mod(SynthSID* sid, uint8_t voice, int enabled);
int synth_sid_get_ring_mod(SynthSID* sid, uint8_t voice);

// Hard sync (voice resets next voice phase)
void synth_sid_set_sync(SynthSID* sid, uint8_t voice, int enabled);
int synth_sid_get_sync(SynthSID* sid, uint8_t voice);

// Pitch bend (MIDI-style, -1.0 to +1.0, maps to ±12 semitones by default)
void synth_sid_set_pitch_bend(SynthSID* sid, uint8_t voice, float bend);
float synth_sid_get_pitch_bend(SynthSID* sid, uint8_t voice);

// TEST bit (resets oscillator phase, silences voice)
void synth_sid_set_test(SynthSID* sid, uint8_t voice, int enabled);
int synth_sid_get_test(SynthSID* sid, uint8_t voice);

// ============================================================================
// Filter Parameters
// ============================================================================

void synth_sid_set_filter_mode(SynthSID* sid, SIDFilterMode mode);
SIDFilterMode synth_sid_get_filter_mode(SynthSID* sid);

void synth_sid_set_filter_cutoff(SynthSID* sid, float cutoff);    // 0.0-1.0
float synth_sid_get_filter_cutoff(SynthSID* sid);

void synth_sid_set_filter_resonance(SynthSID* sid, float resonance); // 0.0-1.0
float synth_sid_get_filter_resonance(SynthSID* sid);

// Filter routing (which voices go through filter)
void synth_sid_set_filter_voice(SynthSID* sid, uint8_t voice, int enabled);
int synth_sid_get_filter_voice(SynthSID* sid, uint8_t voice);

// ============================================================================
// Global Parameters
// ============================================================================

void synth_sid_set_volume(SynthSID* sid, float volume); // 0.0-1.0
float synth_sid_get_volume(SynthSID* sid);

#ifdef __cplusplus
}
#endif

#endif // SYNTH_SID_H
