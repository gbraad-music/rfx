/*
 * WebAssembly Bindings for RGResonate1 Synthesizer
 * Provides wrapper functions for JavaScript integration
 */

#include <emscripten.h>
#include <stdlib.h>
#include <string.h>
#include "synth/synth_resonate1.h"

// Wrapper functions that JavaScript expects (regroove_synth_* interface)

EMSCRIPTEN_KEEPALIVE
SynthResonate1* regroove_synth_create(int engine, float sample_rate) {
    // engine parameter ignored - always create RGResonate1
    return synth_resonate1_create(sample_rate);
}

EMSCRIPTEN_KEEPALIVE
void regroove_synth_destroy(SynthResonate1* synth) {
    synth_resonate1_destroy(synth);
}

EMSCRIPTEN_KEEPALIVE
void regroove_synth_reset(SynthResonate1* synth) {
    synth_resonate1_reset(synth);
}

EMSCRIPTEN_KEEPALIVE
void regroove_synth_note_on(SynthResonate1* synth, uint8_t note, uint8_t velocity) {
    synth_resonate1_note_on(synth, note, velocity);
}

EMSCRIPTEN_KEEPALIVE
void regroove_synth_note_off(SynthResonate1* synth, uint8_t note) {
    synth_resonate1_note_off(synth, note);
}

EMSCRIPTEN_KEEPALIVE
void regroove_synth_control_change(SynthResonate1* synth, uint8_t controller, uint8_t value) {
    // Not implemented in RGResonate1
}

EMSCRIPTEN_KEEPALIVE
void regroove_synth_pitch_bend(SynthResonate1* synth, int value) {
    // Not implemented in RGResonate1
}

EMSCRIPTEN_KEEPALIVE
void regroove_synth_all_notes_off(SynthResonate1* synth) {
    synth_resonate1_all_notes_off(synth);
}

EMSCRIPTEN_KEEPALIVE
void regroove_synth_process_f32(SynthResonate1* synth, float* buffer, int frames, float sample_rate) {
    synth_resonate1_process_f32(synth, buffer, frames, sample_rate);
}

// Parameter interface stubs (not fully implemented yet)
EMSCRIPTEN_KEEPALIVE
int regroove_synth_get_parameter_count(SynthResonate1* synth) {
    return 0;  // TODO: Implement parameter count
}

EMSCRIPTEN_KEEPALIVE
float regroove_synth_get_parameter(SynthResonate1* synth, int index) {
    return 0.0f;  // TODO: Implement parameter get
}

EMSCRIPTEN_KEEPALIVE
void regroove_synth_set_parameter(SynthResonate1* synth, int index, float value) {
    // TODO: Implement parameter set
}

EMSCRIPTEN_KEEPALIVE
const char* regroove_synth_get_parameter_name(int index) {
    return "";  // TODO: Implement parameter names
}

EMSCRIPTEN_KEEPALIVE
const char* regroove_synth_get_parameter_label(int index) {
    return "";  // TODO: Implement parameter labels
}

EMSCRIPTEN_KEEPALIVE
float regroove_synth_get_parameter_default(int index) {
    return 0.0f;  // TODO: Implement parameter defaults
}

EMSCRIPTEN_KEEPALIVE
float regroove_synth_get_parameter_min(int index) {
    return 0.0f;  // TODO: Implement parameter min
}

EMSCRIPTEN_KEEPALIVE
float regroove_synth_get_parameter_max(int index) {
    return 1.0f;  // TODO: Implement parameter max
}

EMSCRIPTEN_KEEPALIVE
int regroove_synth_get_parameter_group(int index) {
    return 0;  // TODO: Implement parameter groups
}

EMSCRIPTEN_KEEPALIVE
const char* regroove_synth_get_group_name(int group) {
    return "";  // TODO: Implement group names
}

EMSCRIPTEN_KEEPALIVE
int regroove_synth_parameter_is_integer(int index) {
    return 0;  // TODO: Implement parameter type
}

EMSCRIPTEN_KEEPALIVE
int regroove_synth_get_engine(SynthResonate1* synth) {
    return 0;  // Always RESONATE1
}

EMSCRIPTEN_KEEPALIVE
const char* regroove_synth_get_engine_name(int engine) {
    return "RGResonate1";
}

// Helper: Create audio buffer for JavaScript
EMSCRIPTEN_KEEPALIVE
void* synth_create_audio_buffer(int frames) {
    // Stereo interleaved buffer
    return malloc(frames * 2 * sizeof(float));
}

// Helper: Destroy audio buffer
EMSCRIPTEN_KEEPALIVE
void synth_destroy_audio_buffer(void* buffer) {
    free(buffer);
}

// Helper: Get buffer size in bytes
EMSCRIPTEN_KEEPALIVE
int synth_get_buffer_size_bytes(int frames) {
    return frames * 2 * sizeof(float);  // Stereo interleaved
}
