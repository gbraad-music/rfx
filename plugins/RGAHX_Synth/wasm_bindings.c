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

    // Trigger note on
    synth->voice_notes[voice_idx] = note;
    ahx_instrument_note_on(&synth->voices[voice_idx], note, velocity, synth->sample_rate);
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

    // Mix all active voices
    float voice_buffer[frames];
    for (int v = 0; v < MAX_VOICES; v++) {
        if (ahx_instrument_is_active(&synth->voices[v])) {
            // Process voice (mono)
            ahx_instrument_process(&synth->voices[v], voice_buffer, frames, sample_rate);

            // Mix to stereo output (duplicate mono to both channels)
            for (int i = 0; i < frames; i++) {
                buffer[i * 2 + 0] += voice_buffer[i];  // Left
                buffer[i * 2 + 1] += voice_buffer[i];  // Right
            }
        }
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
    // Parameter indices match plugin DistrhoPluginInfo.h exactly
    for (int i = 0; i < MAX_VOICES; i++) {
        AhxInstrumentParams* params = &synth->voices[i].params;
        bool recalc_adsr = false;

        switch (index) {
            // Oscillator Group
            case 0:  // kParameterWaveform
                params->waveform = (uint8_t)value;
                synth->voices[i].core_inst.Waveform = (uint8_t)value;
                break;
            case 1:  // kParameterWaveLength
                params->wave_length = (uint8_t)value;
                synth->voices[i].core_inst.WaveLength = (uint8_t)value;
                break;
            case 2:  // kParameterOscVolume
                params->volume = (uint8_t)value;
                synth->voices[i].core_inst.Volume = (uint8_t)value;
                break;

            // Envelope Group
            case 3:  // kParameterAttackFrames
                params->envelope.attack_frames = (uint8_t)value;
                synth->voices[i].core_inst.Envelope.aFrames = (uint8_t)value;
                recalc_adsr = true;
                break;
            case 4:  // kParameterAttackVolume
                params->envelope.attack_volume = (uint8_t)value;
                synth->voices[i].core_inst.Envelope.aVolume = (uint8_t)value;
                recalc_adsr = true;
                break;
            case 5:  // kParameterDecayFrames
                params->envelope.decay_frames = (uint8_t)value;
                synth->voices[i].core_inst.Envelope.dFrames = (uint8_t)value;
                recalc_adsr = true;
                break;
            case 6:  // kParameterDecayVolume
                params->envelope.decay_volume = (uint8_t)value;
                synth->voices[i].core_inst.Envelope.dVolume = (uint8_t)value;
                recalc_adsr = true;
                break;
            case 7:  // kParameterSustainFrames
                params->envelope.sustain_frames = (uint8_t)value;
                synth->voices[i].core_inst.Envelope.sFrames = (uint8_t)value;
                recalc_adsr = true;
                break;
            case 8:  // kParameterReleaseFrames
                params->envelope.release_frames = (uint8_t)value;
                synth->voices[i].core_inst.Envelope.rFrames = (uint8_t)value;
                recalc_adsr = true;
                break;
            case 9:  // kParameterReleaseVolume
                params->envelope.release_volume = (uint8_t)value;
                synth->voices[i].core_inst.Envelope.rVolume = (uint8_t)value;
                recalc_adsr = true;
                break;

            // Filter Group
            case 10:  // kParameterFilterLower
                params->filter_lower = (uint8_t)value;
                synth->voices[i].core_inst.FilterLowerLimit = (uint8_t)value;
                break;
            case 11:  // kParameterFilterUpper
                params->filter_upper = (uint8_t)value;
                synth->voices[i].core_inst.FilterUpperLimit = (uint8_t)value;
                break;
            case 12:  // kParameterFilterSpeed
                params->filter_speed = (uint8_t)value;
                synth->voices[i].core_inst.FilterSpeed = (uint8_t)value;
                break;
            case 13:  // kParameterFilterEnable
                params->filter_enabled = value > 0.5f;
                break;

            // PWM Group
            case 14:  // kParameterSquareLower
                params->square_lower = (uint8_t)value;
                synth->voices[i].core_inst.SquareLowerLimit = (uint8_t)value;
                break;
            case 15:  // kParameterSquareUpper
                params->square_upper = (uint8_t)value;
                synth->voices[i].core_inst.SquareUpperLimit = (uint8_t)value;
                break;
            case 16:  // kParameterSquareSpeed
                params->square_speed = (uint8_t)value;
                synth->voices[i].core_inst.SquareSpeed = (uint8_t)value;
                break;
            case 17:  // kParameterSquareEnable
                params->square_enabled = value > 0.5f;
                break;

            // Vibrato Group
            case 18:  // kParameterVibratoDelay
                params->vibrato_delay = (uint8_t)value;
                synth->voices[i].core_inst.VibratoDelay = (uint8_t)value;
                break;
            case 19:  // kParameterVibratoDepth
                params->vibrato_depth = (uint8_t)value;
                synth->voices[i].core_inst.VibratoDepth = (uint8_t)value;
                break;
            case 20:  // kParameterVibratoSpeed
                params->vibrato_speed = (uint8_t)value;
                synth->voices[i].core_inst.VibratoSpeed = (uint8_t)value;
                break;

            // Release Group
            case 21:  // kParameterHardCutRelease
                params->hard_cut_release = value > 0.5f;
                synth->voices[i].core_inst.HardCutRelease = value > 0.5f ? 1 : 0;
                break;
            case 22:  // kParameterHardCutFrames
                params->hard_cut_frames = (uint8_t)value;
                synth->voices[i].core_inst.HardCutReleaseFrames = (uint8_t)value;
                break;

            // case 23 is kParameterMasterVolume - handled separately at instance level
        }

        // Recalculate ADSR if any envelope parameter changed
        if (recalc_adsr) {
            ahx_synth_voice_calc_adsr(&synth->voices[i].voice, &synth->voices[i].core_inst);
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
