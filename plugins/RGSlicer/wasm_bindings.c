/*
 * WebAssembly Bindings for RGSlicer - Slicing Sampler
 * Loads WAV files with CUE points and maps slices to MIDI notes 36-99
 */

#include <emscripten.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdio.h>
#include "../../synth/rgslicer.h"

#define PARAM_COUNT 13

typedef struct {
    RGSlicer* slicer;
    float sample_rate;

    // Parameters
    float master_volume;    // 0
    float master_pitch;     // 1
    float master_time;      // 2
    float slice_mode;       // 3 (0=transient, 1=zero, 2=grid, 3=bpm)
    float num_slices;       // 4
    float sensitivity;      // 5

    // Slice 0 parameters (as example)
    float s0_pitch;         // 6
    float s0_time;          // 7
    float s0_volume;        // 8
    float s0_pan;           // 9
    float s0_loop;          // 10
    float s0_one_shot;      // 11

    // Sequencer
    float bpm;              // 12
} RGSlicerWASM;

static const char* param_names[PARAM_COUNT] = {
    "Master Volume",
    "Master Pitch",
    "Master Time",
    "Slice Mode",
    "Num Slices",
    "Sensitivity",
    "S0 Pitch",
    "S0 Time",
    "S0 Volume",
    "S0 Pan",
    "S0 Loop",
    "S0 One-Shot",
    "BPM"
};

static const char* param_groups[PARAM_COUNT] = {
    "Master",
    "Master",
    "Master",
    "Slicing",
    "Slicing",
    "Slicing",
    "Slice 0",
    "Slice 0",
    "Slice 0",
    "Slice 0",
    "Slice 0",
    "Slice 0",
    "Sequencer"
};

EMSCRIPTEN_KEEPALIVE
RGSlicerWASM* regroove_synth_create(int engine, float sample_rate) {
    (void)engine;

    RGSlicerWASM* synth = (RGSlicerWASM*)malloc(sizeof(RGSlicerWASM));
    if (!synth) return NULL;

    memset(synth, 0, sizeof(RGSlicerWASM));

    synth->slicer = rgslicer_create((uint32_t)sample_rate);
    if (!synth->slicer) {
        free(synth);
        return NULL;
    }

    synth->sample_rate = sample_rate;

    // Default parameters
    synth->master_volume = 0.7f;
    synth->master_pitch = 0.0f;
    synth->master_time = 1.0f;
    synth->slice_mode = 0.0f;  // Transient detection
    synth->num_slices = 16.0f;
    synth->sensitivity = 0.5f;
    synth->s0_pitch = 0.0f;
    synth->s0_time = 1.0f;
    synth->s0_volume = 1.0f;
    synth->s0_pan = 0.0f;

    rgslicer_set_global_volume(synth->slicer, synth->master_volume);

    printf("[RGSlicerWASM] Created @ %.0f Hz\n", sample_rate);

    return synth;
}

EMSCRIPTEN_KEEPALIVE
void regroove_synth_destroy(RGSlicerWASM* synth) {
    if (!synth) return;

    if (synth->slicer) {
        rgslicer_destroy(synth->slicer);
    }

    free(synth);
}

EMSCRIPTEN_KEEPALIVE
void regroove_synth_reset(RGSlicerWASM* synth) {
    if (!synth || !synth->slicer) return;
    rgslicer_reset(synth->slicer);
}

EMSCRIPTEN_KEEPALIVE
void regroove_synth_note_on(RGSlicerWASM* synth, uint8_t note, uint8_t velocity) {
    if (!synth || !synth->slicer) return;
    rgslicer_note_on(synth->slicer, note, velocity);
}

EMSCRIPTEN_KEEPALIVE
void regroove_synth_note_off(RGSlicerWASM* synth, uint8_t note) {
    if (!synth || !synth->slicer) return;
    rgslicer_note_off(synth->slicer, note);
}

EMSCRIPTEN_KEEPALIVE
void regroove_synth_control_change(RGSlicerWASM* synth, uint8_t controller, uint8_t value) {
    if (!synth || !synth->slicer) return;
    // Handle common CC messages if needed
    (void)controller;
    (void)value;
}

EMSCRIPTEN_KEEPALIVE
void regroove_synth_pitch_bend(RGSlicerWASM* synth, int16_t bend) {
    if (!synth || !synth->slicer) return;
    // Pitch bend not implemented for slicing sampler
    (void)bend;
}

EMSCRIPTEN_KEEPALIVE
void regroove_synth_all_notes_off(RGSlicerWASM* synth) {
    if (!synth || !synth->slicer) return;
    rgslicer_all_notes_off(synth->slicer);
}

EMSCRIPTEN_KEEPALIVE
void regroove_synth_process_f32(RGSlicerWASM* synth, float* buffer, uint32_t frames) {
    if (!synth || !synth->slicer || !buffer) return;
    rgslicer_process_f32(synth->slicer, buffer, frames);
}

EMSCRIPTEN_KEEPALIVE
int regroove_synth_get_parameter_count(RGSlicerWASM* synth) {
    (void)synth;
    return PARAM_COUNT;
}

EMSCRIPTEN_KEEPALIVE
float regroove_synth_get_parameter(RGSlicerWASM* synth, int index) {
    if (!synth || index < 0 || index >= PARAM_COUNT) return 0.0f;

    switch (index) {
        case 0: return synth->master_volume;
        case 1: return synth->master_pitch;
        case 2: return synth->master_time;
        case 3: return synth->slice_mode;
        case 4: return synth->num_slices;
        case 5: return synth->sensitivity;
        case 6: return synth->s0_pitch;
        case 7: return synth->s0_time;
        case 8: return synth->s0_volume;
        case 9: return synth->s0_pan;
        case 10: return synth->s0_loop;
        case 11: return synth->s0_one_shot;
        case 12: return synth->bpm;
        default: return 0.0f;
    }
}

EMSCRIPTEN_KEEPALIVE
void regroove_synth_set_parameter(RGSlicerWASM* synth, int index, float value) {
    if (!synth || !synth->slicer || index < 0 || index >= PARAM_COUNT) return;

    switch (index) {
        case 0: // Master Volume
            synth->master_volume = value;
            rgslicer_set_global_volume(synth->slicer, value);
            break;

        case 1: // Master Pitch
            synth->master_pitch = value;
            rgslicer_set_global_pitch(synth->slicer, value * 12.0f - 6.0f);  // -6 to +6 semitones
            break;

        case 2: // Master Time
            synth->master_time = value;
            rgslicer_set_global_time(synth->slicer, value * 1.5f + 0.5f);  // 0.5x to 2.0x
            break;

        case 3: // Slice Mode
            synth->slice_mode = value;
            if (rgslicer_has_sample(synth->slicer)) {
                SliceMode mode = (SliceMode)((int)(value * 3.0f));
                rgslicer_auto_slice(synth->slicer, mode, (uint8_t)synth->num_slices, synth->sensitivity);
            }
            break;

        case 4: // Num Slices
            synth->num_slices = value;
            if (rgslicer_has_sample(synth->slicer)) {
                SliceMode mode = (SliceMode)((int)synth->slice_mode);
                rgslicer_auto_slice(synth->slicer, mode, (uint8_t)(value * 63.0f + 1.0f), synth->sensitivity);
            }
            break;

        case 5: // Sensitivity
            synth->sensitivity = value;
            if (rgslicer_has_sample(synth->slicer)) {
                SliceMode mode = (SliceMode)((int)synth->slice_mode);
                rgslicer_auto_slice(synth->slicer, mode, (uint8_t)synth->num_slices, value);
            }
            break;

        case 6: // S0 Pitch
            synth->s0_pitch = value;
            rgslicer_set_slice_pitch(synth->slicer, 0, value * 24.0f - 12.0f);  // -12 to +12 semitones
            break;

        case 7: // S0 Time
            synth->s0_time = value;
            rgslicer_set_slice_time(synth->slicer, 0, value * 1.5f + 0.5f);  // 0.5x to 2.0x
            break;

        case 8: // S0 Volume
            synth->s0_volume = value;
            rgslicer_set_slice_volume(synth->slicer, 0, value * 2.0f);  // 0.0 to 2.0
            break;

        case 9: // S0 Pan
            synth->s0_pan = value;
            rgslicer_set_slice_pan(synth->slicer, 0, value * 2.0f - 1.0f);  // -1.0 to +1.0
            break;

        case 10: // S0 Loop
            synth->s0_loop = value;
            rgslicer_set_slice_loop(synth->slicer, 0, value > 0.5f);
            break;

        case 11: // S0 One-Shot
            synth->s0_one_shot = value;
            rgslicer_set_slice_one_shot(synth->slicer, 0, value > 0.5f);
            break;

        case 12: // BPM
            synth->bpm = value;
            rgslicer_set_bpm(synth->slicer, value);
            break;
    }
}

EMSCRIPTEN_KEEPALIVE
const char* regroove_synth_get_parameter_name(RGSlicerWASM* synth, int index) {
    (void)synth;
    if (index < 0 || index >= PARAM_COUNT) return "";
    return param_names[index];
}

EMSCRIPTEN_KEEPALIVE
const char* regroove_synth_get_parameter_label(RGSlicerWASM* synth, int index) {
    (void)synth;
    (void)index;
    return "";  // No units
}

EMSCRIPTEN_KEEPALIVE
float regroove_synth_get_parameter_default(RGSlicerWASM* synth, int index) {
    (void)synth;

    switch (index) {
        case 0: return 0.7f;    // Master Volume
        case 1: return 0.5f;    // Master Pitch (centered)
        case 2: return 0.5f;    // Master Time (1.0x)
        case 3: return 0.0f;    // Slice Mode (transient)
        case 4: return 0.25f;   // Num Slices (16)
        case 5: return 0.5f;    // Sensitivity
        case 6: return 0.5f;    // S0 Pitch (centered)
        case 7: return 0.5f;    // S0 Time (1.0x)
        case 8: return 0.5f;    // S0 Volume (1.0)
        case 9: return 0.5f;    // S0 Pan (center)
        case 10: return 0.0f;   // S0 Loop (off)
        case 11: return 0.0f;   // S0 One-Shot (off)
        case 12: return 125.0f; // BPM (125)
        default: return 0.5f;
    }
}

EMSCRIPTEN_KEEPALIVE
float regroove_synth_get_parameter_min(RGSlicerWASM* synth, int index) {
    (void)synth;
    (void)index;
    return 0.0f;
}

EMSCRIPTEN_KEEPALIVE
float regroove_synth_get_parameter_max(RGSlicerWASM* synth, int index) {
    (void)synth;
    (void)index;
    return 1.0f;
}

EMSCRIPTEN_KEEPALIVE
const char* regroove_synth_get_parameter_group(RGSlicerWASM* synth, int index) {
    (void)synth;
    if (index < 0 || index >= PARAM_COUNT) return "";
    return param_groups[index];
}

EMSCRIPTEN_KEEPALIVE
const char* regroove_synth_get_group_name(RGSlicerWASM* synth, const char* group) {
    (void)synth;
    return group;  // Return group name as-is
}

EMSCRIPTEN_KEEPALIVE
int regroove_synth_parameter_is_integer(RGSlicerWASM* synth, int index) {
    (void)synth;
    // Slice mode (3), num slices (4), loop (10), one-shot (11), BPM (12) are integers
    return (index == 3 || index == 4 || index == 10 || index == 11 || index == 12) ? 1 : 0;
}

EMSCRIPTEN_KEEPALIVE
int regroove_synth_get_engine(RGSlicerWASM* synth) {
    (void)synth;
    return 0;  // Only one engine
}

EMSCRIPTEN_KEEPALIVE
const char* regroove_synth_get_engine_name(RGSlicerWASM* synth, int engine) {
    (void)synth;
    (void)engine;
    return "Slicer";
}

// Audio buffer helpers
EMSCRIPTEN_KEEPALIVE
float* synth_create_audio_buffer(uint32_t frames) {
    return (float*)malloc(frames * 2 * sizeof(float));  // Stereo
}

EMSCRIPTEN_KEEPALIVE
void synth_destroy_audio_buffer(float* buffer) {
    if (buffer) free(buffer);
}

EMSCRIPTEN_KEEPALIVE
int synth_get_buffer_size_bytes(uint32_t frames) {
    return frames * 2 * sizeof(float);
}

// WAV file loading from JavaScript
// NOTE: Does NOT auto-slice - caller (worklet) decides based on CUE points
EMSCRIPTEN_KEEPALIVE
int rgslicer_load_wav_from_memory(RGSlicerWASM* synth, int16_t* pcm_data, uint32_t num_samples, uint32_t sample_rate) {
    if (!synth || !synth->slicer || !pcm_data) return 0;

    int result = rgslicer_load_sample_memory(synth->slicer, pcm_data, num_samples, sample_rate);

    if (result) {
        printf("[RGSlicerWASM] Loaded %u samples @ %u Hz\n", num_samples, sample_rate);
        // DO NOT auto-slice here - worklet will slice based on CUE points or manual trigger
    }

    return result;
}

EMSCRIPTEN_KEEPALIVE
uint8_t rgslicer_get_slice_count(RGSlicerWASM* synth) {
    if (!synth || !synth->slicer) return 0;
    return rgslicer_get_num_slices(synth->slicer);
}

EMSCRIPTEN_KEEPALIVE
uint32_t rgslicer_get_slice_offset_at(RGSlicerWASM* synth, uint8_t index) {
    if (!synth || !synth->slicer) return 0;
    return rgslicer_get_slice_offset(synth->slicer, index);
}

EMSCRIPTEN_KEEPALIVE
uint32_t rgslicer_get_slice_length_at(RGSlicerWASM* synth, uint8_t index) {
    if (!synth || !synth->slicer) return 0;
    return rgslicer_get_slice_length(synth->slicer, index);
}

// Set slices from CUE points OR auto-slice if no CUE points provided
// This function handles the decision logic in C code (not JavaScript)
EMSCRIPTEN_KEEPALIVE
int rgslicer_set_slices_from_cues(RGSlicerWASM* synth, uint32_t* positions, uint32_t num_positions) {
    if (!synth || !synth->slicer) return 0;

    if (positions && num_positions > 0) {
        // User provided CUE points - use them (NO auto-slicing)
        printf("[RGSlicerWASM] Using %u CUE points from WAV file\n", num_positions);

        // Clear existing slices and note mapping
        rgslicer_clear_slices(synth->slicer);

        for (uint32_t i = 0; i < num_positions; i++) {
            rgslicer_add_slice(synth->slicer, positions[i]);
        }

        printf("[RGSlicerWASM] Created %u slices from CUE points\n", num_positions);
        return (int)num_positions;
    } else {
        // No CUE points - run auto-slice with current parameters
        printf("[RGSlicerWASM] No CUE points - running auto-slice\n");

        SliceMode mode = (SliceMode)((int)synth->slice_mode);
        uint8_t num_slices = (uint8_t)(synth->num_slices * 63.0f + 1.0f);

        int result = rgslicer_auto_slice(synth->slicer, mode, num_slices, synth->sensitivity);
        printf("[RGSlicerWASM] Auto-sliced: %d slices created\n", result);

        return result;
    }
}

// Set slices with MIDI note assignments and loop flags from CUE point labels
// positions: sample positions for each slice
// notes: MIDI note assignments (from CUE labels like "64" or "64-loop")
// loops: loop flags (1 if label ends with "-loop", 0 otherwise)
//
// IMPORTANT: Audio BEFORE the first CUE marker is NOT assigned to any slice!
// If first CUE is at position 60096, audio from 0-60095 will be unplayable.
// To include beginning: Add CUE marker at position 0 in your audio editor.
EMSCRIPTEN_KEEPALIVE
int rgslicer_set_slices_with_notes(RGSlicerWASM* synth, uint32_t* positions, uint8_t* notes, uint8_t* loops, uint32_t num_slices) {
    if (!synth || !synth->slicer || !positions || !notes || !loops) return 0;

    printf("[RGSlicerWASM] Loading %u slices with note assignments\n", num_slices);

    // Clear existing slices and note mapping
    rgslicer_clear_slices(synth->slicer);

    // Create slices and build note-to-slice mapping
    for (uint32_t i = 0; i < num_slices; i++) {
        int slice_idx = rgslicer_add_slice(synth->slicer, positions[i]);
        if (slice_idx >= 0) {
            // Set loop flag from CUE label
            rgslicer_set_slice_loop(synth->slicer, (uint8_t)slice_idx, loops[i] != 0);

            // Map MIDI note to this slice
            uint8_t midi_note = notes[i];
            if (midi_note < 128) {
                synth->slicer->note_map[midi_note] = (uint8_t)slice_idx;
            }

            printf("[RGSlicerWASM]   Slice %d: pos=%u note=%u loop=%d\n",
                   slice_idx, positions[i], midi_note, loops[i]);
        }
    }

    // Enable note mapping (CUE labels provided explicit note assignments)
    synth->slicer->use_note_map = 1;

    printf("[RGSlicerWASM] Created %u slices with note mappings\n", num_slices);
    return (int)num_slices;
}
