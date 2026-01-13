/*
 * WebAssembly Bindings for RGAHX Synthesizer
 * Provides wrapper functions for JavaScript integration
 */

#include <emscripten.h>
#include <stdlib.h>
#include <string.h>
#include "synth/ahx_instrument.h"

// Simple polyphonic wrapper (max 8 voices)
#define MAX_VOICES 8

typedef struct {
    AhxInstrument voices[MAX_VOICES];
    float sample_rate;
    uint8_t voice_notes[MAX_VOICES];  // Track which note each voice is playing
} AhxSynthInstance;

// Wrapper functions that JavaScript expects (regroove_synth_* interface)

EMSCRIPTEN_KEEPALIVE
AhxSynthInstance* regroove_synth_create(int engine, float sample_rate) {
    // engine parameter ignored - always create AHX
    AhxSynthInstance* instance = (AhxSynthInstance*)malloc(sizeof(AhxSynthInstance));
    if (!instance) return NULL;

    instance->sample_rate = sample_rate;

    // Initialize all voices
    for (int i = 0; i < MAX_VOICES; i++) {
        ahx_instrument_init(&instance->voices[i]);
        instance->voice_notes[i] = 0xFF;  // 0xFF = not playing
    }

    return instance;
}

EMSCRIPTEN_KEEPALIVE
void regroove_synth_destroy(AhxSynthInstance* synth) {
    if (synth) {
        free(synth);
    }
}

EMSCRIPTEN_KEEPALIVE
void regroove_synth_reset(AhxSynthInstance* synth) {
    if (!synth) return;

    for (int i = 0; i < MAX_VOICES; i++) {
        ahx_instrument_reset(&synth->voices[i]);
        synth->voice_notes[i] = 0xFF;
    }
}

EMSCRIPTEN_KEEPALIVE
void regroove_synth_note_on(AhxSynthInstance* synth, uint8_t note, uint8_t velocity) {
    if (!synth) {
        emscripten_log(EM_LOG_ERROR, "[RGAHX] note_on: synth is NULL!");
        return;
    }

    // Find free voice or steal oldest
    int voice_idx = -1;

    // First try to find a free voice
    for (int i = 0; i < MAX_VOICES; i++) {
        if (!ahx_instrument_is_active(&synth->voices[i])) {
            voice_idx = i;
            break;
        }
    }

    // If no free voice, steal the first one
    if (voice_idx == -1) {
        voice_idx = 0;
    }

    emscripten_log(EM_LOG_CONSOLE, "[RGAHX] Note ON: %d vel:%d on voice %d", note, velocity, voice_idx);

    // Trigger note on
    synth->voice_notes[voice_idx] = note;
    ahx_instrument_note_on(&synth->voices[voice_idx], note, velocity, synth->sample_rate);

    emscripten_log(EM_LOG_CONSOLE, "[RGAHX] Voice active: %d", ahx_instrument_is_active(&synth->voices[voice_idx]));
}

EMSCRIPTEN_KEEPALIVE
void regroove_synth_note_off(AhxSynthInstance* synth, uint8_t note) {
    if (!synth) return;

    // Find voice playing this note and release it
    for (int i = 0; i < MAX_VOICES; i++) {
        if (synth->voice_notes[i] == note) {
            ahx_instrument_note_off(&synth->voices[i]);
            synth->voice_notes[i] = 0xFF;
        }
    }
}

EMSCRIPTEN_KEEPALIVE
void regroove_synth_control_change(AhxSynthInstance* synth, uint8_t controller, uint8_t value) {
    // Not implemented yet - could map CC to AHX parameters
}

EMSCRIPTEN_KEEPALIVE
void regroove_synth_pitch_bend(AhxSynthInstance* synth, int value) {
    // Not implemented yet
}

EMSCRIPTEN_KEEPALIVE
void regroove_synth_all_notes_off(AhxSynthInstance* synth) {
    if (!synth) return;

    for (int i = 0; i < MAX_VOICES; i++) {
        if (synth->voice_notes[i] != 0xFF) {
            ahx_instrument_note_off(&synth->voices[i]);
            synth->voice_notes[i] = 0xFF;
        }
    }
}

EMSCRIPTEN_KEEPALIVE
void regroove_synth_process_f32(AhxSynthInstance* synth, float* buffer, int frames, float sample_rate) {
    if (!synth || !buffer) return;

    // Clear buffer
    memset(buffer, 0, frames * 2 * sizeof(float));

    static int log_counter = 0;
    int active_voices = 0;
    float max_sample = 0.0f;

    // Mix all active voices
    float voice_buffer[frames];
    for (int v = 0; v < MAX_VOICES; v++) {
        if (ahx_instrument_is_active(&synth->voices[v])) {
            active_voices++;

            // Process voice (mono)
            ahx_instrument_process(&synth->voices[v], voice_buffer, frames, sample_rate);

            // Mix to stereo output (duplicate mono to both channels)
            for (int i = 0; i < frames; i++) {
                buffer[i * 2 + 0] += voice_buffer[i];  // Left
                buffer[i * 2 + 1] += voice_buffer[i];  // Right

                float abs_sample = voice_buffer[i] < 0 ? -voice_buffer[i] : voice_buffer[i];
                if (abs_sample > max_sample) max_sample = abs_sample;
            }
        }
    }

    // Log every 100 calls
    if (++log_counter % 100 == 0 && active_voices > 0) {
        emscripten_log(EM_LOG_CONSOLE, "[RGAHX] Active:%d MaxSample:%.6f", active_voices, max_sample);
    }
}

// Parameter interface stubs (not fully implemented yet)
EMSCRIPTEN_KEEPALIVE
int regroove_synth_get_parameter_count(AhxSynthInstance* synth) {
    return 0;  // TODO: Implement parameter count
}

EMSCRIPTEN_KEEPALIVE
float regroove_synth_get_parameter(AhxSynthInstance* synth, int index) {
    return 0.0f;  // TODO: Implement parameter get
}

EMSCRIPTEN_KEEPALIVE
void regroove_synth_set_parameter(AhxSynthInstance* synth, int index, float value) {
    if (!synth) return;

    // Update all voices with new parameters
    for (int i = 0; i < MAX_VOICES; i++) {
        AhxInstrumentParams* params = &synth->voices[i].params;

        switch (index) {
            case 0: // Waveform
                params->waveform = (uint8_t)value;
                synth->voices[i].core_inst.Waveform = (uint8_t)value;
                emscripten_log(EM_LOG_CONSOLE, "[RGAHX] Waveform set to %d", (int)value);
                break;

            case 1: // Attack frames
                params->envelope.attack_frames = (uint8_t)value;
                synth->voices[i].core_inst.Envelope.aFrames = (uint8_t)value;
                ahx_synth_voice_calc_adsr(&synth->voices[i].voice, &synth->voices[i].core_inst);
                emscripten_log(EM_LOG_CONSOLE, "[RGAHX] Attack set to %d", (int)value);
                break;

            case 2: // Decay frames
                params->envelope.decay_frames = (uint8_t)value;
                synth->voices[i].core_inst.Envelope.dFrames = (uint8_t)value;
                ahx_synth_voice_calc_adsr(&synth->voices[i].voice, &synth->voices[i].core_inst);
                emscripten_log(EM_LOG_CONSOLE, "[RGAHX] Decay set to %d", (int)value);
                break;

            case 3: // Sustain frames
                params->envelope.sustain_frames = (uint8_t)value;
                synth->voices[i].core_inst.Envelope.sFrames = (uint8_t)value;
                ahx_synth_voice_calc_adsr(&synth->voices[i].voice, &synth->voices[i].core_inst);
                emscripten_log(EM_LOG_CONSOLE, "[RGAHX] Sustain set to %d", (int)value);
                break;

            case 4: // Release frames
                params->envelope.release_frames = (uint8_t)value;
                synth->voices[i].core_inst.Envelope.rFrames = (uint8_t)value;
                ahx_synth_voice_calc_adsr(&synth->voices[i].voice, &synth->voices[i].core_inst);
                emscripten_log(EM_LOG_CONSOLE, "[RGAHX] Release set to %d", (int)value);
                break;
        }
    }
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
int regroove_synth_get_engine(AhxSynthInstance* synth) {
    return 1;  // AHX engine ID
}

EMSCRIPTEN_KEEPALIVE
const char* regroove_synth_get_engine_name(int engine) {
    return "RGAHX";
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
