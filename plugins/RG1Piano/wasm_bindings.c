/*
 * WebAssembly Bindings for RG1Piano Synthesizer
 * M1 Piano with modal synthesis
 */

#include <emscripten.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "../../synth/synth_modal_piano.h"
#include "../../data/rg1piano/sample_data.h"

#define MAX_VOICES 8

typedef struct {
    ModalPiano* voices[MAX_VOICES];
    SampleData sample_data;
    bool voice_active[MAX_VOICES];
    uint8_t voice_note[MAX_VOICES];

    // Parameters (0-1 range)
    float decay;
    float resonance;
    float brightness;
    float velocity_sens;
    float volume;
    float lfo_rate;
    float lfo_depth;
} RG1PianoWASM;

// Parameter indices
enum {
    PARAM_DECAY = 0,
    PARAM_RESONANCE,
    PARAM_BRIGHTNESS,
    PARAM_VELOCITY_SENS,
    PARAM_VOLUME,
    PARAM_LFO_RATE,
    PARAM_LFO_DEPTH,
    PARAM_COUNT
};

static const char* param_names[PARAM_COUNT] = {
    "Decay",
    "Resonance",
    "Brightness",
    "Vel Sens",
    "Volume",
    "LFO Rate",
    "LFO Depth"
};

static void update_voice_params(RG1PianoWASM* synth, int voice_idx) {
    if (voice_idx < 0 || voice_idx >= MAX_VOICES || !synth->voices[voice_idx]) return;

    ModalPiano* piano = synth->voices[voice_idx];

    // Map decay parameter to 0.5s - 8s range
    float decay_time = 0.5f + synth->decay * 7.5f;
    modal_piano_set_decay(piano, decay_time);

    modal_piano_set_resonance(piano, synth->resonance);
    modal_piano_set_filter_envelope(piano, 0.01f, 0.3f, synth->brightness);
    modal_piano_set_velocity_sensitivity(piano, synth->velocity_sens);

    // Map LFO rate: 0-1 -> 0.5Hz-8Hz
    float lfo_freq = 0.5f + synth->lfo_rate * 7.5f;
    modal_piano_set_lfo(piano, lfo_freq, synth->lfo_depth);
}

EMSCRIPTEN_KEEPALIVE
RG1PianoWASM* regroove_synth_create(int engine, float sample_rate) {
    (void)engine;
    (void)sample_rate;

    RG1PianoWASM* synth = (RG1PianoWASM*)malloc(sizeof(RG1PianoWASM));
    if (!synth) return NULL;

    memset(synth, 0, sizeof(RG1PianoWASM));

    // Initialize sample data
    synth->sample_data.attack_data = m1piano_onset;
    synth->sample_data.attack_length = m1piano_onset_length;
    synth->sample_data.loop_data = m1piano_tail;
    synth->sample_data.loop_length = m1piano_tail_length;
    synth->sample_data.sample_rate = M1PIANO_SAMPLE_RATE;
    synth->sample_data.root_note = M1PIANO_ROOT_NOTE;

    // Create voices
    for (int i = 0; i < MAX_VOICES; i++) {
        synth->voices[i] = modal_piano_create();
        if (synth->voices[i]) {
            modal_piano_load_sample(synth->voices[i], &synth->sample_data);
        }
        synth->voice_active[i] = false;
        synth->voice_note[i] = 0;
    }

    // Default parameters
    synth->decay = 0.5f;
    synth->resonance = 0.0f;
    synth->brightness = 0.6f;
    synth->velocity_sens = 0.8f;
    synth->volume = 0.7f;
    synth->lfo_rate = 0.3f;
    synth->lfo_depth = 0.2f;

    // Update all voices
    for (int i = 0; i < MAX_VOICES; i++) {
        update_voice_params(synth, i);
    }

    return synth;
}

EMSCRIPTEN_KEEPALIVE
void regroove_synth_destroy(RG1PianoWASM* synth) {
    if (!synth) return;

    for (int i = 0; i < MAX_VOICES; i++) {
        if (synth->voices[i]) {
            modal_piano_destroy(synth->voices[i]);
        }
    }

    free(synth);
}

EMSCRIPTEN_KEEPALIVE
void regroove_synth_reset(RG1PianoWASM* synth) {
    if (!synth) return;

    for (int i = 0; i < MAX_VOICES; i++) {
        if (synth->voices[i]) {
            modal_piano_reset(synth->voices[i]);
        }
        synth->voice_active[i] = false;
        synth->voice_note[i] = 0;
    }
}

static int find_free_voice(RG1PianoWASM* synth) {
    // First try to find an inactive voice
    for (int i = 0; i < MAX_VOICES; i++) {
        if (!synth->voice_active[i]) return i;
    }

    // Otherwise steal oldest voice (voice 0)
    return 0;
}

EMSCRIPTEN_KEEPALIVE
void regroove_synth_note_on(RG1PianoWASM* synth, uint8_t note, uint8_t velocity) {
    if (!synth) return;

    int voice_idx = find_free_voice(synth);
    if (voice_idx < 0 || !synth->voices[voice_idx]) return;

    modal_piano_trigger(synth->voices[voice_idx], note, velocity);
    synth->voice_active[voice_idx] = true;
    synth->voice_note[voice_idx] = note;
}

EMSCRIPTEN_KEEPALIVE
void regroove_synth_note_off(RG1PianoWASM* synth, uint8_t note) {
    if (!synth) return;

    for (int i = 0; i < MAX_VOICES; i++) {
        if (synth->voice_active[i] && synth->voice_note[i] == note) {
            modal_piano_release(synth->voices[i]);
        }
    }
}

EMSCRIPTEN_KEEPALIVE
void regroove_synth_control_change(RG1PianoWASM* synth, uint8_t controller, uint8_t value) {
    // Not implemented
    (void)synth;
    (void)controller;
    (void)value;
}

EMSCRIPTEN_KEEPALIVE
void regroove_synth_pitch_bend(RG1PianoWASM* synth, int value) {
    // Not implemented
    (void)synth;
    (void)value;
}

EMSCRIPTEN_KEEPALIVE
void regroove_synth_all_notes_off(RG1PianoWASM* synth) {
    if (!synth) return;

    for (int i = 0; i < MAX_VOICES; i++) {
        if (synth->voice_active[i] && synth->voices[i]) {
            modal_piano_release(synth->voices[i]);
        }
    }
}

EMSCRIPTEN_KEEPALIVE
void regroove_synth_process_f32(RG1PianoWASM* synth, float* buffer, int frames, float sample_rate) {
    if (!synth || !buffer) return;

    // Clear buffer
    memset(buffer, 0, frames * 2 * sizeof(float));

    // Process each voice
    for (int i = 0; i < MAX_VOICES; i++) {
        if (!synth->voice_active[i] || !synth->voices[i]) continue;

        for (int frame = 0; frame < frames; frame++) {
            float sample = modal_piano_process(synth->voices[i], (uint32_t)sample_rate);

            // Check if voice is still active
            if (!modal_piano_is_active(synth->voices[i])) {
                synth->voice_active[i] = false;
            }

            // Mix to stereo interleaved buffer with volume
            buffer[frame * 2 + 0] += sample * synth->volume * 0.3f;  // Left
            buffer[frame * 2 + 1] += sample * synth->volume * 0.3f;  // Right
        }
    }

    // Soft clipping
    for (int i = 0; i < frames * 2; i++) {
        if (buffer[i] > 1.0f) {
            buffer[i] = 1.0f - expf(-(buffer[i] - 1.0f));
        } else if (buffer[i] < -1.0f) {
            buffer[i] = -1.0f + expf(buffer[i] + 1.0f);
        }
    }
}

// Parameter interface
EMSCRIPTEN_KEEPALIVE
int regroove_synth_get_parameter_count(RG1PianoWASM* synth) {
    (void)synth;
    return PARAM_COUNT;
}

EMSCRIPTEN_KEEPALIVE
float regroove_synth_get_parameter(RG1PianoWASM* synth, int index) {
    if (!synth || index < 0 || index >= PARAM_COUNT) return 0.0f;

    switch (index) {
        case PARAM_DECAY: return synth->decay;
        case PARAM_RESONANCE: return synth->resonance;
        case PARAM_BRIGHTNESS: return synth->brightness;
        case PARAM_VELOCITY_SENS: return synth->velocity_sens;
        case PARAM_VOLUME: return synth->volume;
        case PARAM_LFO_RATE: return synth->lfo_rate;
        case PARAM_LFO_DEPTH: return synth->lfo_depth;
        default: return 0.0f;
    }
}

EMSCRIPTEN_KEEPALIVE
void regroove_synth_set_parameter(RG1PianoWASM* synth, int index, float value) {
    if (!synth || index < 0 || index >= PARAM_COUNT) return;

    switch (index) {
        case PARAM_DECAY:
            synth->decay = value;
            break;
        case PARAM_RESONANCE:
            synth->resonance = value;
            break;
        case PARAM_BRIGHTNESS:
            synth->brightness = value;
            break;
        case PARAM_VELOCITY_SENS:
            synth->velocity_sens = value;
            break;
        case PARAM_VOLUME:
            synth->volume = value;
            return;  // Don't update voices for volume
        case PARAM_LFO_RATE:
            synth->lfo_rate = value;
            break;
        case PARAM_LFO_DEPTH:
            synth->lfo_depth = value;
            break;
    }

    // Update all voices with new parameters
    for (int i = 0; i < MAX_VOICES; i++) {
        update_voice_params(synth, i);
    }
}

EMSCRIPTEN_KEEPALIVE
const char* regroove_synth_get_parameter_name(int index) {
    if (index < 0 || index >= PARAM_COUNT) return "";
    return param_names[index];
}

EMSCRIPTEN_KEEPALIVE
const char* regroove_synth_get_parameter_label(int index) {
    (void)index;
    return "";
}

EMSCRIPTEN_KEEPALIVE
float regroove_synth_get_parameter_default(int index) {
    switch (index) {
        case PARAM_DECAY: return 0.5f;
        case PARAM_RESONANCE: return 0.0f;
        case PARAM_BRIGHTNESS: return 0.6f;
        case PARAM_VELOCITY_SENS: return 0.8f;
        case PARAM_VOLUME: return 0.7f;
        case PARAM_LFO_RATE: return 0.3f;
        case PARAM_LFO_DEPTH: return 0.2f;
        default: return 0.0f;
    }
}

EMSCRIPTEN_KEEPALIVE
float regroove_synth_get_parameter_min(int index) {
    (void)index;
    return 0.0f;
}

EMSCRIPTEN_KEEPALIVE
float regroove_synth_get_parameter_max(int index) {
    (void)index;
    return 1.0f;
}

EMSCRIPTEN_KEEPALIVE
int regroove_synth_get_parameter_group(int index) {
    if (index <= PARAM_VELOCITY_SENS) return 0;  // Synthesis group
    return 1;  // Modulation & Output group
}

EMSCRIPTEN_KEEPALIVE
const char* regroove_synth_get_group_name(int group) {
    switch (group) {
        case 0: return "Synthesis";
        case 1: return "Modulation & Output";
        default: return "";
    }
}

EMSCRIPTEN_KEEPALIVE
int regroove_synth_parameter_is_integer(int index) {
    (void)index;
    return 0;  // All parameters are float
}

EMSCRIPTEN_KEEPALIVE
int regroove_synth_get_engine(RG1PianoWASM* synth) {
    (void)synth;
    return 2;  // RG1Piano engine ID (0=RG909, 1=RGResonate1, 2=RG1Piano)
}

EMSCRIPTEN_KEEPALIVE
const char* regroove_synth_get_engine_name(int engine) {
    if (engine == 2) return "RG1Piano";
    return "Unknown";
}

// Helper: Create audio buffer for JavaScript
EMSCRIPTEN_KEEPALIVE
void* synth_create_audio_buffer(int frames) {
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
    return frames * 2 * sizeof(float);
}
