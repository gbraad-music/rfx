/*
 * MIDIbox SID V2 Compatibility Layer
 * Provides MIDIbox SID V2 behavior for all platforms (WASM, VST3, Logue)
 */

#ifndef SYNTH_SID_MIDIBOX_H
#define SYNTH_SID_MIDIBOX_H

#include <stdint.h>
#include "synth_sid.h"

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// MIDIbox SID V2 Engine Modes
// ============================================================================

typedef enum {
    MIDIBOX_ENGINE_LEAD = 0,    // All 3 voices play in unison on MIDI channel 1
    MIDIBOX_ENGINE_MULTI = 1    // Independent voices on MIDI channels 1, 2, 3
} MIDIboxEngineMode;

// ============================================================================
// MIDIbox Preset Structure (matches web/data/ format)
// ============================================================================

typedef struct {
    const char* name;
    uint8_t waveform;
    float pulseWidth;
    float attack, decay, sustain, release;
    uint8_t filterMode;
    float filterCutoff, filterResonance;
    uint8_t filterVoice1;
    uint8_t filterVoice2;
    uint8_t filterVoice3;
} MIDIboxPreset;

// ============================================================================
// MIDIbox SID Instance
// ============================================================================

typedef struct {
    SynthSID* sid;
    float sample_rate;
    float parameters[40];         // Parameter cache for UI sync
    MIDIboxEngineMode engine_mode;
} MIDIboxSIDInstance;

// ============================================================================
// Core Functions
// ============================================================================

MIDIboxSIDInstance* midibox_sid_create(float sample_rate);
void midibox_sid_destroy(MIDIboxSIDInstance* mb);
void midibox_sid_reset(MIDIboxSIDInstance* mb);

// ============================================================================
// Engine Mode
// ============================================================================

void midibox_sid_set_engine_mode(MIDIboxSIDInstance* mb, MIDIboxEngineMode mode);
MIDIboxEngineMode midibox_sid_get_engine_mode(MIDIboxSIDInstance* mb);

// ============================================================================
// MIDI Message Handling (MIDIbox SID V2 compatible routing)
// ============================================================================

void midibox_sid_handle_midi(MIDIboxSIDInstance* mb, uint8_t status, uint8_t data1, uint8_t data2);
void midibox_sid_note_on(MIDIboxSIDInstance* mb, uint8_t channel, uint8_t note, uint8_t velocity);
void midibox_sid_note_off(MIDIboxSIDInstance* mb, uint8_t channel, uint8_t note);
void midibox_sid_control_change(MIDIboxSIDInstance* mb, uint8_t channel, uint8_t controller, uint8_t value);
void midibox_sid_pitch_bend(MIDIboxSIDInstance* mb, uint8_t channel, uint16_t value);
void midibox_sid_all_notes_off(MIDIboxSIDInstance* mb);

// ============================================================================
// Audio Processing
// ============================================================================

void midibox_sid_process_f32(MIDIboxSIDInstance* mb, float* buffer, int frames, float sample_rate);

// ============================================================================
// Parameter Management (40 parameters total)
// ============================================================================

int midibox_sid_get_parameter_count(void);
float midibox_sid_get_parameter(MIDIboxSIDInstance* mb, int index);
void midibox_sid_set_parameter(MIDIboxSIDInstance* mb, int index, float value);
const char* midibox_sid_get_parameter_name(int index);
const char* midibox_sid_get_parameter_label(int index);
float midibox_sid_get_parameter_default(int index);
float midibox_sid_get_parameter_min(int index);
float midibox_sid_get_parameter_max(int index);
int midibox_sid_get_parameter_group(int index);
const char* midibox_sid_get_group_name(int group);
int midibox_sid_parameter_is_integer(int index);

// ============================================================================
// Preset System
// ============================================================================

int midibox_sid_get_preset_count(void);
const char* midibox_sid_get_preset_name(int index);
void midibox_sid_load_preset(MIDIboxSIDInstance* mb, int index, int voice);

#ifdef __cplusplus
}
#endif

#endif // SYNTH_SID_MIDIBOX_H
