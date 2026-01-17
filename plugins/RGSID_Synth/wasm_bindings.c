/*
 * WebAssembly Bindings for RGSID Synthesizer
 * Thin wrapper around the shared MIDIbox SID compatibility layer
 */

#include <emscripten.h>
#include <stdlib.h>
#include "synth/synth_sid_midibox.h"

// ============================================================================
// Core Functions
// ============================================================================

EMSCRIPTEN_KEEPALIVE
MIDIboxSIDInstance* regroove_synth_create(int engine, float sample_rate) {
    // engine parameter ignored - always create SID
    return midibox_sid_create(sample_rate);
}

EMSCRIPTEN_KEEPALIVE
void regroove_synth_destroy(MIDIboxSIDInstance* synth) {
    midibox_sid_destroy(synth);
}

EMSCRIPTEN_KEEPALIVE
void regroove_synth_reset(MIDIboxSIDInstance* synth) {
    midibox_sid_reset(synth);
}

// ============================================================================
// Simple Note API (for backward compatibility)
// ============================================================================

EMSCRIPTEN_KEEPALIVE
void regroove_synth_note_on(MIDIboxSIDInstance* synth, uint8_t note, uint8_t velocity) {
    // Default to channel 0 for simple API
    midibox_sid_note_on(synth, 0, note, velocity);
}

EMSCRIPTEN_KEEPALIVE
void regroove_synth_note_off(MIDIboxSIDInstance* synth, uint8_t note) {
    // Default to channel 0 for simple API
    midibox_sid_note_off(synth, 0, note);
}

// ============================================================================
// MIDI Message Handling
// ============================================================================

EMSCRIPTEN_KEEPALIVE
void regroove_synth_handle_midi(MIDIboxSIDInstance* synth, uint8_t status, uint8_t data1, uint8_t data2) {
    midibox_sid_handle_midi(synth, status, data1, data2);
}

EMSCRIPTEN_KEEPALIVE
void regroove_synth_control_change(MIDIboxSIDInstance* synth, uint8_t controller, uint8_t value) {
    midibox_sid_control_change(synth, 0, controller, value);
}

EMSCRIPTEN_KEEPALIVE
void regroove_synth_pitch_bend(MIDIboxSIDInstance* synth, int value) {
    midibox_sid_pitch_bend(synth, 0, (uint16_t)value);
}

EMSCRIPTEN_KEEPALIVE
void regroove_synth_all_notes_off(MIDIboxSIDInstance* synth) {
    midibox_sid_all_notes_off(synth);
}

// ============================================================================
// Audio Processing
// ============================================================================

EMSCRIPTEN_KEEPALIVE
void regroove_synth_process_f32(MIDIboxSIDInstance* synth, float* buffer, int frames, float sample_rate) {
    midibox_sid_process_f32(synth, buffer, frames, sample_rate);
}

// ============================================================================
// Parameter Interface
// ============================================================================

EMSCRIPTEN_KEEPALIVE
int regroove_synth_get_parameter_count(MIDIboxSIDInstance* synth) {
    return midibox_sid_get_parameter_count();
}

EMSCRIPTEN_KEEPALIVE
float regroove_synth_get_parameter(MIDIboxSIDInstance* synth, int index) {
    return midibox_sid_get_parameter(synth, index);
}

EMSCRIPTEN_KEEPALIVE
void regroove_synth_set_parameter(MIDIboxSIDInstance* synth, int index, float value) {
    midibox_sid_set_parameter(synth, index, value);
}

EMSCRIPTEN_KEEPALIVE
const char* regroove_synth_get_parameter_name(int index) {
    return midibox_sid_get_parameter_name(index);
}

EMSCRIPTEN_KEEPALIVE
const char* regroove_synth_get_parameter_label(int index) {
    return midibox_sid_get_parameter_label(index);
}

EMSCRIPTEN_KEEPALIVE
float regroove_synth_get_parameter_default(int index) {
    return midibox_sid_get_parameter_default(index);
}

EMSCRIPTEN_KEEPALIVE
float regroove_synth_get_parameter_min(int index) {
    return midibox_sid_get_parameter_min(index);
}

EMSCRIPTEN_KEEPALIVE
float regroove_synth_get_parameter_max(int index) {
    return midibox_sid_get_parameter_max(index);
}

EMSCRIPTEN_KEEPALIVE
int regroove_synth_get_parameter_group(int index) {
    return midibox_sid_get_parameter_group(index);
}

EMSCRIPTEN_KEEPALIVE
const char* regroove_synth_get_group_name(int group) {
    return midibox_sid_get_group_name(group);
}

EMSCRIPTEN_KEEPALIVE
int regroove_synth_parameter_is_integer(int index) {
    return midibox_sid_parameter_is_integer(index);
}

// ============================================================================
// Engine Info
// ============================================================================

EMSCRIPTEN_KEEPALIVE
int regroove_synth_get_engine(MIDIboxSIDInstance* synth) {
    return 2;  // SID engine ID
}

EMSCRIPTEN_KEEPALIVE
const char* regroove_synth_get_engine_name(int engine) {
    return "RGSID";
}

// ============================================================================
// Audio Buffer Helpers
// ============================================================================

EMSCRIPTEN_KEEPALIVE
void* synth_create_audio_buffer(int frames) {
    return malloc(frames * 2 * sizeof(float));
}

EMSCRIPTEN_KEEPALIVE
void synth_destroy_audio_buffer(void* buffer) {
    free(buffer);
}

EMSCRIPTEN_KEEPALIVE
int synth_get_buffer_size_bytes(int frames) {
    return frames * 2 * sizeof(float);
}

// ============================================================================
// Preset System
// ============================================================================

EMSCRIPTEN_KEEPALIVE
int regroove_synth_get_preset_count() {
    return midibox_sid_get_preset_count();
}

EMSCRIPTEN_KEEPALIVE
const char* regroove_synth_get_preset_name(int index) {
    return midibox_sid_get_preset_name(index);
}

EMSCRIPTEN_KEEPALIVE
void regroove_synth_load_preset(MIDIboxSIDInstance* synth, int index, int voice) {
    midibox_sid_load_preset(synth, index, voice);
}
