/*
 * WebAssembly Bindings for RGSID Synthesizer
 * Provides wrapper functions for JavaScript integration
 */

#include <emscripten.h>
#include <stdlib.h>
#include <string.h>
#include "synth/synth_sid.h"
#include "synth/synth_sid_cc.h"

// Simple wrapper for SID synth
typedef struct {
    SynthSID* sid;
    float sample_rate;
    float parameters[39];  // Cache of current parameter values (for UI sync)
} SIDSynthInstance;

// Wrapper functions that JavaScript expects (regroove_synth_* interface)

EMSCRIPTEN_KEEPALIVE
SIDSynthInstance* regroove_synth_create(int engine, float sample_rate) {
    // engine parameter ignored - always create SID
    SIDSynthInstance* instance = (SIDSynthInstance*)malloc(sizeof(SIDSynthInstance));
    if (!instance) return NULL;

    instance->sample_rate = sample_rate;
    instance->sid = synth_sid_create((int)sample_rate);

    if (!instance->sid) {
        free(instance);
        return NULL;
    }

    // Initialize parameter cache to defaults
    memset(instance->parameters, 0, sizeof(instance->parameters));

    return instance;
}

EMSCRIPTEN_KEEPALIVE
void regroove_synth_destroy(SIDSynthInstance* synth) {
    if (synth) {
        if (synth->sid) {
            synth_sid_destroy(synth->sid);
        }
        free(synth);
    }
}

EMSCRIPTEN_KEEPALIVE
void regroove_synth_reset(SIDSynthInstance* synth) {
    if (!synth || !synth->sid) return;
    synth_sid_reset(synth->sid);
}

EMSCRIPTEN_KEEPALIVE
void regroove_synth_note_on(SIDSynthInstance* synth, uint8_t note, uint8_t velocity) {
    if (!synth || !synth->sid) {
        emscripten_log(EM_LOG_ERROR, "[RGSID] note_on: synth is NULL!");
        return;
    }

    // Default to Voice 1 for simple note_on API
    synth_sid_note_on(synth->sid, 0, note, velocity);
}

EMSCRIPTEN_KEEPALIVE
void regroove_synth_note_off(SIDSynthInstance* synth, uint8_t note) {
    if (!synth || !synth->sid) return;

    // Default to Voice 1 for simple note_off API
    synth_sid_note_off(synth->sid, 0);
}

// Proper MIDI message handler with channel routing (MIDIbox SID V2 compatible)
EMSCRIPTEN_KEEPALIVE
void regroove_synth_handle_midi(SIDSynthInstance* synth, uint8_t status, uint8_t data1, uint8_t data2) {
    if (!synth || !synth->sid) return;

    const uint8_t message = status & 0xF0;
    const uint8_t channel = status & 0x0F;

    // Route MIDI channels to voices (MIDIbox SID V2 style)
    // Channel 0 (MIDI 1) → Voice 0
    // Channel 1 (MIDI 2) → Voice 1
    // Channel 2 (MIDI 3) → Voice 2
    // Channels 9-15 → Voice 0 (omni fallback)
    uint8_t voice = 0;
    if (channel < 3) {
        voice = channel;
    }

    switch (message) {
        case 0x90: // Note On
            if (data2 > 0) {
                synth_sid_note_on(synth->sid, voice, data1, data2);
            } else {
                synth_sid_note_off(synth->sid, voice);
            }
            break;

        case 0x80: // Note Off
            synth_sid_note_off(synth->sid, voice);
            break;

        case 0xB0: // Control Change
            synth_sid_handle_cc(synth->sid, data1, data2);
            break;

        case 0xE0: // Pitch Bend
            {
                uint16_t bend = (data2 << 7) | data1;
                synth_sid_handle_pitch_bend_midi(synth->sid, voice, bend);
            }
            break;
    }
}

EMSCRIPTEN_KEEPALIVE
void regroove_synth_control_change(SIDSynthInstance* synth, uint8_t controller, uint8_t value) {
    if (!synth || !synth->sid) return;

    // Use the SID CC handler for MIDIbox compatibility
    synth_sid_handle_cc(synth->sid, controller, value);
}

EMSCRIPTEN_KEEPALIVE
void regroove_synth_pitch_bend(SIDSynthInstance* synth, int value) {
    if (!synth || !synth->sid) return;

    // Apply pitch bend to all voices
    for (int i = 0; i < 3; i++) {
        synth_sid_handle_pitch_bend_midi(synth->sid, i, (uint16_t)value);
    }
}

EMSCRIPTEN_KEEPALIVE
void regroove_synth_all_notes_off(SIDSynthInstance* synth) {
    if (!synth || !synth->sid) return;
    synth_sid_all_notes_off(synth->sid);
}

EMSCRIPTEN_KEEPALIVE
void regroove_synth_process_f32(SIDSynthInstance* synth, float* buffer, int frames, float sample_rate) {
    if (!synth || !synth->sid || !buffer) return;

    // SID synth already outputs stereo interleaved
    synth_sid_process_f32(synth->sid, buffer, frames, (int)sample_rate);
}

// Parameter interface - maps to SID synth parameters
// Total parameters: 39 (8 per voice x 3 + 7 filter/global + 8 LFO)
EMSCRIPTEN_KEEPALIVE
int regroove_synth_get_parameter_count(SIDSynthInstance* synth) {
    return 39;
}

EMSCRIPTEN_KEEPALIVE
float regroove_synth_get_parameter(SIDSynthInstance* synth, int index) {
    if (!synth || index < 0 || index >= 39) return 0.0f;
    return synth->parameters[index];
}

EMSCRIPTEN_KEEPALIVE
void regroove_synth_set_parameter(SIDSynthInstance* synth, int index, float value) {
    if (!synth || !synth->sid) return;

    // Cache parameter value for UI sync
    if (index >= 0 && index < 39) {
        synth->parameters[index] = value;
    }

    // Parameter mapping (matches RGSID_SynthPlugin.cpp):
    // Voice 1: 0-7, Voice 2: 8-15, Voice 3: 16-23, Filter: 24-28

    int voice = index / 8;  // Which voice (0-2)
    int param = index % 8;  // Which parameter within voice

    if (voice < 3) {
        // Voice parameters
        switch (param) {
            case 0:  // Waveform
                synth_sid_set_waveform(synth->sid, voice, (uint8_t)value);
                break;
            case 1:  // Pulse Width
                synth_sid_set_pulse_width(synth->sid, voice, value);
                break;
            case 2:  // Attack
                synth_sid_set_attack(synth->sid, voice, value);
                break;
            case 3:  // Decay
                synth_sid_set_decay(synth->sid, voice, value);
                break;
            case 4:  // Sustain
                synth_sid_set_sustain(synth->sid, voice, value);
                break;
            case 5:  // Release
                synth_sid_set_release(synth->sid, voice, value);
                break;
            case 6:  // Ring Mod
                synth_sid_set_ring_mod(synth->sid, voice, value > 0.5f);
                break;
            case 7:  // Sync
                synth_sid_set_sync(synth->sid, voice, value > 0.5f);
                break;
        }
    } else {
        // Filter and global parameters (24-30)
        switch (index) {
            case 24:  // Filter Mode
                synth_sid_set_filter_mode(synth->sid, (SIDFilterMode)(int)value);
                break;
            case 25:  // Filter Cutoff
                synth_sid_set_filter_cutoff(synth->sid, value);
                break;
            case 26:  // Filter Resonance
                synth_sid_set_filter_resonance(synth->sid, value);
                break;
            case 27:  // Filter Voice 1
                synth_sid_set_filter_voice(synth->sid, 0, value > 0.5f);
                break;
            case 28:  // Filter Voice 2
                synth_sid_set_filter_voice(synth->sid, 1, value > 0.5f);
                break;
            case 29:  // Filter Voice 3
                synth_sid_set_filter_voice(synth->sid, 2, value > 0.5f);
                break;
            case 30:  // Volume
                synth_sid_set_volume(synth->sid, value);
                break;
            case 31:  // LFO1 Rate
                synth_sid_set_lfo_frequency(synth->sid, 0, 0.1f * powf(100.0f, value));
                break;
            case 32:  // LFO1 Waveform
                synth_sid_set_lfo_waveform(synth->sid, 0, (int)value);
                break;
            case 33:  // LFO1 → Pitch
                synth_sid_set_lfo1_to_pitch(synth->sid, value);
                break;
            case 34:  // LFO2 Rate
                synth_sid_set_lfo_frequency(synth->sid, 1, 0.05f * powf(100.0f, value));
                break;
            case 35:  // LFO2 Waveform
                synth_sid_set_lfo_waveform(synth->sid, 1, (int)value);
                break;
            case 36:  // LFO2 → Filter
                synth_sid_set_lfo2_to_filter(synth->sid, value);
                break;
            case 37:  // LFO2 → PW
                synth_sid_set_lfo2_to_pw(synth->sid, value);
                break;
            case 38:  // Mod Wheel
                synth_sid_set_mod_wheel(synth->sid, value);
                break;
        }
    }
}

EMSCRIPTEN_KEEPALIVE
const char* regroove_synth_get_parameter_name(int index) {
    static const char* names[] = {
        // Voice 1 (0-7)
        "V1 Waveform", "V1 Pulse Width", "V1 Attack", "V1 Decay",
        "V1 Sustain", "V1 Release", "V1 Ring Mod", "V1 Sync",
        // Voice 2 (8-15)
        "V2 Waveform", "V2 Pulse Width", "V2 Attack", "V2 Decay",
        "V2 Sustain", "V2 Release", "V2 Ring Mod", "V2 Sync",
        // Voice 3 (16-23)
        "V3 Waveform", "V3 Pulse Width", "V3 Attack", "V3 Decay",
        "V3 Sustain", "V3 Release", "V3 Ring Mod", "V3 Sync",
        // Filter/Global (24-30)
        "Filter Mode", "Filter Cutoff", "Filter Resonance",
        "Filter V1", "Filter V2", "Filter V3", "Volume",
        // LFO (31-38)
        "LFO1 Rate", "LFO1 Waveform", "LFO1 → Pitch",
        "LFO2 Rate", "LFO2 Waveform", "LFO2 → Filter", "LFO2 → PW",
        "Mod Wheel"
    };

    if (index < 0 || index >= 39) return "";
    return names[index];
}

EMSCRIPTEN_KEEPALIVE
const char* regroove_synth_get_parameter_label(int index) {
    return "";  // No units for most parameters
}

EMSCRIPTEN_KEEPALIVE
float regroove_synth_get_parameter_default(int index) {
    // Simple defaults
    if (index % 8 == 0) return 4.0f;  // Waveform: Pulse
    if (index % 8 == 1) return 0.5f;  // Pulse Width: 50%
    if (index % 8 == 4) return 0.7f;  // Sustain: 70%
    if (index == 28) return 0.7f;      // Volume: 70%
    return 0.0f;
}

EMSCRIPTEN_KEEPALIVE
float regroove_synth_get_parameter_min(int index) {
    return 0.0f;
}

EMSCRIPTEN_KEEPALIVE
float regroove_synth_get_parameter_max(int index) {
    if (index % 8 == 0) return 15.0f;  // Waveform (bitfield)
    if (index == 24) return 3.0f;       // Filter mode (0-3)
    return 1.0f;
}

EMSCRIPTEN_KEEPALIVE
int regroove_synth_get_parameter_group(int index) {
    if (index < 8) return 0;   // Voice 1
    if (index < 16) return 1;  // Voice 2
    if (index < 24) return 2;  // Voice 3
    return 3;                   // Filter/Global
}

EMSCRIPTEN_KEEPALIVE
const char* regroove_synth_get_group_name(int group) {
    static const char* groups[] = { "Voice 1", "Voice 2", "Voice 3", "Filter/Global" };
    if (group < 0 || group >= 4) return "";
    return groups[group];
}

EMSCRIPTEN_KEEPALIVE
int regroove_synth_parameter_is_integer(int index) {
    // Waveform and filter mode are integers
    if (index % 8 == 0 || index == 24) return 1;
    // Ring mod and sync are booleans (treated as integer)
    if (index % 8 == 6 || index % 8 == 7) return 1;
    return 0;
}

EMSCRIPTEN_KEEPALIVE
int regroove_synth_get_engine(SIDSynthInstance* synth) {
    return 2;  // SID engine ID (different from AHX which is 1)
}

EMSCRIPTEN_KEEPALIVE
const char* regroove_synth_get_engine_name(int engine) {
    return "RGSID";
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

// ============================================================================
// Preset System (Factory Presets for Web)
// ============================================================================

typedef struct {
    const char* name;
    uint8_t waveform;
    float pulseWidth;
    float attack, decay, sustain, release;
    uint8_t filterMode;
    float filterCutoff, filterResonance;
    uint8_t filterVoice1;
} WebPreset;

static const WebPreset factoryPresets[] = {
    // ===== INIT & BASICS (0-7) =====
    { "Init", 4, 0.5f, 0.0f, 0.5f, 0.7f, 0.3f, 1, 0.5f, 0.0f, 0 },
    { "Saw Lead", 2, 0.5f, 0.0f, 0.3f, 0.6f, 0.2f, 1, 0.6f, 0.3f, 1 },
    { "Pulse Lead", 4, 0.5f, 0.0f, 0.3f, 0.6f, 0.2f, 1, 0.6f, 0.3f, 1 },
    { "Triangle Lead", 1, 0.5f, 0.0f, 0.3f, 0.6f, 0.2f, 1, 0.6f, 0.3f, 1 },
    { "Noise Lead", 8, 0.5f, 0.0f, 0.3f, 0.6f, 0.2f, 1, 0.6f, 0.3f, 1 },
    { "Square 25%", 4, 0.25f, 0.0f, 0.3f, 0.6f, 0.2f, 1, 0.6f, 0.3f, 1 },
    { "Square 12.5%", 4, 0.12f, 0.0f, 0.3f, 0.6f, 0.2f, 1, 0.6f, 0.3f, 1 },
    { "Square 75%", 4, 0.75f, 0.0f, 0.3f, 0.6f, 0.2f, 1, 0.6f, 0.3f, 1 },

    // ===== BASS SOUNDS (8-23) =====
    { "Pulse Bass", 4, 0.25f, 0.0f, 0.4f, 0.3f, 0.1f, 1, 0.4f, 0.5f, 1 },
    { "Saw Bass", 2, 0.5f, 0.0f, 0.4f, 0.3f, 0.1f, 1, 0.35f, 0.6f, 1 },
    { "Triangle Bass", 1, 0.5f, 0.0f, 0.4f, 0.3f, 0.15f, 1, 0.3f, 0.4f, 1 },
    { "Sync Bass", 2, 0.5f, 0.0f, 0.3f, 0.5f, 0.1f, 1, 0.3f, 0.7f, 1 },
    { "Resonant Bass", 4, 0.5f, 0.0f, 0.5f, 0.2f, 0.05f, 1, 0.25f, 0.9f, 1 },
    { "Acid Bass", 2, 0.5f, 0.0f, 0.5f, 0.3f, 0.1f, 1, 0.2f, 0.85f, 1 },
    { "Deep Bass", 1, 0.5f, 0.0f, 0.6f, 0.0f, 0.1f, 1, 0.15f, 0.3f, 1 },
    { "Sync Wobble", 2, 0.5f, 0.0f, 0.4f, 0.4f, 0.2f, 1, 0.3f, 0.8f, 1 },
    { "Seq Bass 1", 4, 0.3f, 0.0f, 0.4f, 0.3f, 0.05f, 1, 0.35f, 0.7f, 1 },
    { "Seq Bass 2", 2, 0.5f, 0.0f, 0.35f, 0.4f, 0.1f, 1, 0.3f, 0.75f, 1 },
    { "Funky Bass", 4, 0.4f, 0.0f, 0.2f, 0.5f, 0.15f, 1, 0.4f, 0.6f, 1 },
    { "Noise Bass", 8, 0.5f, 0.0f, 0.3f, 0.4f, 0.1f, 3, 0.4f, 0.5f, 1 },
    { "Reso Pluck", 4, 0.5f, 0.0f, 0.6f, 0.0f, 0.2f, 1, 0.3f, 0.85f, 1 },
    { "Fat Bass", 4, 0.6f, 0.0f, 0.5f, 0.2f, 0.1f, 1, 0.3f, 0.6f, 1 },
    { "Sub Bass", 1, 0.5f, 0.0f, 0.7f, 0.0f, 0.1f, 1, 0.1f, 0.2f, 1 },
    { "Zap Bass", 2, 0.5f, 0.0f, 0.2f, 0.5f, 0.1f, 1, 0.5f, 0.8f, 1 },

    // ===== LEAD SOUNDS (24-39) =====
    { "Brass Lead", 2, 0.5f, 0.0f, 0.3f, 0.6f, 0.2f, 1, 0.6f, 0.3f, 1 },
    { "Sync Lead", 2, 0.5f, 0.0f, 0.2f, 0.8f, 0.1f, 1, 0.7f, 0.2f, 1 },
    { "Pulse Lead", 4, 0.4f, 0.0f, 0.2f, 0.7f, 0.15f, 1, 0.65f, 0.4f, 1 },
    { "Fuzzy Lead", 2, 0.5f, 0.0f, 0.1f, 0.8f, 0.1f, 1, 0.8f, 0.3f, 1 },
    { "Soft Lead", 1, 0.5f, 0.0f, 0.4f, 0.6f, 0.3f, 1, 0.5f, 0.2f, 1 },
    { "Ring Lead", 1, 0.5f, 0.0f, 0.3f, 0.7f, 0.2f, 0, 0.6f, 0.0f, 0 },
    { "Hard Lead", 4, 0.3f, 0.0f, 0.1f, 0.8f, 0.1f, 1, 0.75f, 0.5f, 1 },
    { "Screamer", 2, 0.5f, 0.0f, 0.0f, 0.9f, 0.05f, 1, 0.9f, 0.1f, 1 },
    { "Thin Lead", 4, 0.15f, 0.0f, 0.2f, 0.7f, 0.15f, 1, 0.7f, 0.4f, 1 },
    { "Wide Lead", 4, 0.7f, 0.0f, 0.2f, 0.7f, 0.15f, 1, 0.6f, 0.4f, 1 },
    { "Stabby Lead", 2, 0.5f, 0.0f, 0.1f, 0.7f, 0.05f, 1, 0.65f, 0.5f, 1 },
    { "Mono Lead", 2, 0.5f, 0.0f, 0.3f, 0.6f, 0.2f, 1, 0.55f, 0.4f, 1 },
    { "Reso Lead", 4, 0.5f, 0.0f, 0.2f, 0.7f, 0.15f, 1, 0.4f, 0.85f, 1 },
    { "Pure Lead", 1, 0.5f, 0.0f, 0.2f, 0.7f, 0.15f, 0, 0.5f, 0.0f, 0 },
    { "Dirty Lead", 8, 0.5f, 0.0f, 0.2f, 0.7f, 0.15f, 3, 0.6f, 0.4f, 1 },
    { "Epic Lead", 2, 0.5f, 0.0f, 0.4f, 0.7f, 0.3f, 1, 0.6f, 0.3f, 1 },

    // ===== CLASSIC C64 SOUNDS (40-55) =====
    { "SEQ Vintage C", 2, 0.5f, 0.0f, 0.4f, 0.5f, 0.1f, 1, 0.35f, 0.7f, 1 },
    { "Last Ninja", 2, 0.5f, 0.0f, 0.3f, 0.6f, 0.2f, 1, 0.45f, 0.6f, 1 },
    { "Commando", 4, 0.4f, 0.0f, 0.2f, 0.7f, 0.1f, 1, 0.5f, 0.7f, 1 },
    { "Monty Run", 2, 0.5f, 0.0f, 0.35f, 0.5f, 0.15f, 1, 0.4f, 0.65f, 1 },
    { "Driller", 4, 0.3f, 0.0f, 0.3f, 0.6f, 0.15f, 1, 0.45f, 0.7f, 1 },
    { "Delta", 2, 0.5f, 0.0f, 0.4f, 0.5f, 0.2f, 1, 0.5f, 0.5f, 1 },
    { "Galway Lead", 2, 0.5f, 0.0f, 0.2f, 0.7f, 0.15f, 1, 0.6f, 0.4f, 1 },
    { "Hubbard Bass", 4, 0.3f, 0.0f, 0.4f, 0.3f, 0.1f, 1, 0.35f, 0.75f, 1 },
    { "Tel Bass", 4, 0.25f, 0.0f, 0.5f, 0.2f, 0.05f, 1, 0.3f, 0.8f, 1 },
    { "Game Over", 8, 0.5f, 0.0f, 0.2f, 0.6f, 0.1f, 1, 0.5f, 0.3f, 1 },
    { "Arkanoid", 1, 0.5f, 0.0f, 0.3f, 0.6f, 0.2f, 0, 0.5f, 0.0f, 0 },
    { "Turrican", 2, 0.5f, 0.0f, 0.3f, 0.6f, 0.2f, 1, 0.55f, 0.5f, 1 },
    { "International", 4, 0.4f, 0.0f, 0.25f, 0.6f, 0.15f, 1, 0.5f, 0.6f, 1 },
    { "Ocean Loader", 2, 0.5f, 0.0f, 0.4f, 0.5f, 0.2f, 1, 0.45f, 0.55f, 1 },
    { "Thrust", 4, 0.35f, 0.0f, 0.3f, 0.5f, 0.15f, 1, 0.5f, 0.65f, 1 },
    { "Wizball", 1, 0.5f, 0.0f, 0.4f, 0.5f, 0.25f, 1, 0.4f, 0.4f, 1 },

    // ===== PADS & STRINGS (56-63) =====
    { "Soft Pad", 1, 0.5f, 0.5f, 0.8f, 0.8f, 0.5f, 1, 0.5f, 0.2f, 1 },
    { "Saw Pad", 2, 0.5f, 0.5f, 0.8f, 0.8f, 0.5f, 1, 0.5f, 0.3f, 1 },
    { "Pulse Pad", 4, 0.5f, 0.5f, 0.8f, 0.8f, 0.5f, 1, 0.5f, 0.3f, 1 },
    { "Sync Pad", 2, 0.5f, 0.5f, 0.8f, 0.8f, 0.5f, 0, 0.5f, 0.0f, 0 },
    { "Strings", 2, 0.5f, 0.6f, 0.9f, 0.9f, 0.6f, 1, 0.6f, 0.2f, 1 },
    { "Brass Sect", 2, 0.5f, 0.3f, 0.6f, 0.7f, 0.4f, 1, 0.55f, 0.3f, 1 },
    { "Slow Pad", 1, 0.5f, 0.8f, 0.9f, 0.9f, 0.7f, 1, 0.5f, 0.2f, 1 },
    { "Atmosphere", 8, 0.5f, 0.6f, 0.9f, 0.9f, 0.6f, 2, 0.5f, 0.3f, 1 },

    // ===== PLUCKS & BELLS (64-71) =====
    { "Pluck", 1, 0.5f, 0.0f, 0.5f, 0.0f, 0.3f, 1, 0.5f, 0.2f, 1 },
    { "Harp", 1, 0.5f, 0.0f, 0.6f, 0.0f, 0.4f, 0, 0.5f, 0.0f, 0 },
    { "Marimba", 1, 0.5f, 0.0f, 0.4f, 0.0f, 0.2f, 1, 0.4f, 0.3f, 1 },
    { "Ring Bell", 1, 0.5f, 0.0f, 0.6f, 0.0f, 0.5f, 0, 0.5f, 0.0f, 0 },
    { "Sync Bell", 2, 0.5f, 0.0f, 0.6f, 0.0f, 0.5f, 0, 0.6f, 0.0f, 0 },
    { "Clav", 4, 0.3f, 0.0f, 0.3f, 0.0f, 0.15f, 1, 0.6f, 0.4f, 1 },
    { "Koto", 1, 0.5f, 0.0f, 0.5f, 0.0f, 0.35f, 1, 0.5f, 0.3f, 1 },
    { "Kalimba", 1, 0.5f, 0.0f, 0.4f, 0.0f, 0.25f, 0, 0.5f, 0.0f, 0 },

    // ===== FX & PERCUSSION (72-79) =====
    { "Laser", 2, 0.5f, 0.0f, 0.3f, 0.0f, 0.1f, 1, 0.8f, 0.5f, 1 },
    { "Zap", 8, 0.5f, 0.0f, 0.2f, 0.0f, 0.1f, 3, 0.7f, 0.4f, 1 },
    { "Sweep Up", 2, 0.5f, 0.0f, 0.5f, 0.0f, 0.3f, 1, 0.3f, 0.7f, 1 },
    { "Sweep Down", 2, 0.5f, 0.0f, 0.5f, 0.0f, 0.3f, 1, 0.7f, 0.7f, 1 },
    { "Noise Hit", 8, 0.5f, 0.0f, 0.2f, 0.0f, 0.1f, 3, 0.5f, 0.3f, 1 },
    { "Noise Snare", 8, 0.5f, 0.0f, 0.15f, 0.0f, 0.1f, 3, 0.6f, 0.3f, 1 },
    { "Tom", 1, 0.5f, 0.0f, 0.3f, 0.0f, 0.15f, 1, 0.3f, 0.4f, 1 },
    { "Kick", 1, 0.5f, 0.0f, 0.2f, 0.0f, 0.05f, 1, 0.2f, 0.3f, 1 },

    // ===== SPECIAL (80-87) =====
    { "Digi Bass", 8, 0.5f, 0.0f, 0.4f, 0.3f, 0.1f, 1, 0.3f, 0.5f, 1 },
    { "Voice", 8, 0.5f, 0.3f, 0.6f, 0.7f, 0.4f, 2, 0.5f, 0.3f, 1 },
    { "Choir", 1, 0.5f, 0.5f, 0.8f, 0.8f, 0.6f, 2, 0.6f, 0.2f, 1 },
    { "Organ", 4, 0.5f, 0.1f, 0.5f, 0.7f, 0.3f, 1, 0.5f, 0.3f, 1 },
    { "Accordion", 4, 0.6f, 0.2f, 0.6f, 0.7f, 0.4f, 1, 0.5f, 0.3f, 1 },
    { "Harmonica", 2, 0.5f, 0.1f, 0.5f, 0.7f, 0.3f, 1, 0.5f, 0.3f, 1 },
    { "Flute", 1, 0.5f, 0.3f, 0.6f, 0.7f, 0.4f, 1, 0.6f, 0.2f, 1 },
    { "Sitar", 1, 0.5f, 0.0f, 0.5f, 0.0f, 0.4f, 0, 0.5f, 0.0f, 0 },

    // ===== MORE BASSES (88-95) =====
    { "Tech Bass", 4, 0.35f, 0.0f, 0.3f, 0.4f, 0.1f, 1, 0.3f, 0.75f, 1 },
    { "Wobble Bass", 2, 0.5f, 0.0f, 0.5f, 0.3f, 0.2f, 1, 0.25f, 0.85f, 1 },
    { "Trance Bass", 2, 0.5f, 0.0f, 0.4f, 0.4f, 0.15f, 1, 0.3f, 0.8f, 1 },
    { "Electro Bass", 4, 0.3f, 0.0f, 0.3f, 0.5f, 0.1f, 1, 0.35f, 0.7f, 1 },
    { "Minimal Bass", 1, 0.5f, 0.0f, 0.5f, 0.2f, 0.1f, 1, 0.25f, 0.5f, 1 },
    { "Hard Bass", 2, 0.5f, 0.0f, 0.2f, 0.6f, 0.05f, 1, 0.4f, 0.8f, 1 },
    { "Soft Bass", 1, 0.5f, 0.0f, 0.5f, 0.4f, 0.2f, 1, 0.3f, 0.3f, 1 },
    { "Vintage Bass", 4, 0.4f, 0.0f, 0.4f, 0.4f, 0.15f, 1, 0.35f, 0.6f, 1 },

    // ===== MORE LEADS (96-103) =====
    { "Space Lead", 2, 0.5f, 0.0f, 0.3f, 0.7f, 0.2f, 0, 0.6f, 0.0f, 0 },
    { "Retro Lead", 4, 0.5f, 0.0f, 0.2f, 0.7f, 0.15f, 1, 0.55f, 0.5f, 1 },
    { "Chip Lead", 4, 0.25f, 0.0f, 0.1f, 0.8f, 0.05f, 1, 0.6f, 0.4f, 1 },
    { "8-bit Lead", 4, 0.5f, 0.0f, 0.1f, 0.8f, 0.05f, 0, 0.5f, 0.0f, 0 },
    { "Arpeggio", 4, 0.5f, 0.0f, 0.2f, 0.0f, 0.1f, 1, 0.5f, 0.3f, 1 },
    { "Stab", 2, 0.5f, 0.0f, 0.1f, 0.7f, 0.05f, 1, 0.6f, 0.5f, 1 },
    { "PWM Lead", 4, 0.5f, 0.0f, 0.3f, 0.6f, 0.2f, 1, 0.5f, 0.4f, 1 },
    { "Dirty Sync", 2, 0.5f, 0.0f, 0.1f, 0.8f, 0.05f, 0, 0.7f, 0.0f, 0 },

    // ===== EXPERIMENTAL (104-111) =====
    { "Random 1", 6, 0.5f, 0.2f, 0.5f, 0.5f, 0.3f, 2, 0.5f, 0.4f, 1 },
    { "Random 2", 7, 0.6f, 0.3f, 0.6f, 0.4f, 0.2f, 1, 0.6f, 0.5f, 1 },
    { "Random 3", 5, 0.4f, 0.1f, 0.4f, 0.6f, 0.25f, 3, 0.5f, 0.3f, 1 },
    { "Glitch 1", 8, 0.5f, 0.0f, 0.1f, 0.0f, 0.05f, 3, 0.7f, 0.5f, 1 },
    { "Glitch 2", 8, 0.5f, 0.0f, 0.15f, 0.0f, 0.1f, 2, 0.6f, 0.6f, 1 },
    { "Lo-Fi", 8, 0.5f, 0.2f, 0.5f, 0.5f, 0.3f, 1, 0.5f, 0.4f, 1 },
    { "Crushed", 8, 0.5f, 0.0f, 0.2f, 0.5f, 0.1f, 3, 0.6f, 0.5f, 1 },
    { "Broken", 6, 0.3f, 0.0f, 0.3f, 0.3f, 0.15f, 2, 0.5f, 0.6f, 1 },

    // ===== DRONE & AMBIENT (112-119) =====
    { "Drone 1", 2, 0.5f, 0.8f, 0.9f, 0.9f, 0.8f, 1, 0.4f, 0.2f, 1 },
    { "Drone 2", 1, 0.5f, 0.8f, 0.9f, 0.9f, 0.8f, 0, 0.5f, 0.0f, 0 },
    { "Dark Pad", 2, 0.5f, 0.7f, 0.9f, 0.9f, 0.7f, 1, 0.3f, 0.3f, 1 },
    { "Space Pad", 8, 0.5f, 0.6f, 0.9f, 0.9f, 0.6f, 2, 0.5f, 0.2f, 1 },
    { "Wind", 8, 0.5f, 0.5f, 0.8f, 0.8f, 0.5f, 2, 0.6f, 0.3f, 1 },
    { "Ocean", 8, 0.5f, 0.6f, 0.9f, 0.9f, 0.7f, 1, 0.4f, 0.4f, 1 },
    { "Rain", 8, 0.5f, 0.3f, 0.7f, 0.7f, 0.4f, 3, 0.5f, 0.3f, 1 },
    { "Thunder", 8, 0.5f, 0.0f, 0.3f, 0.0f, 0.2f, 1, 0.3f, 0.5f, 1 },

    // ===== UTILITY & SPECIAL (120-127) =====
    { "Test Tone", 1, 0.5f, 0.0f, 0.0f, 1.0f, 0.0f, 0, 0.5f, 0.0f, 0 },
    { "Click", 1, 0.5f, 0.0f, 0.0f, 0.0f, 0.01f, 0, 0.5f, 0.0f, 0 },
    { "Pop", 4, 0.5f, 0.0f, 0.0f, 0.0f, 0.02f, 1, 0.5f, 0.0f, 1 },
    { "Beep", 4, 0.5f, 0.0f, 0.1f, 0.0f, 0.05f, 0, 0.5f, 0.0f, 0 },
    { "Chirp", 1, 0.5f, 0.0f, 0.2f, 0.0f, 0.1f, 1, 0.7f, 0.3f, 1 },
    { "Blip", 4, 0.25f, 0.0f, 0.1f, 0.0f, 0.05f, 1, 0.6f, 0.2f, 1 },
    { "Silence", 0, 0.5f, 0.0f, 0.0f, 0.0f, 0.0f, 0, 0.0f, 0.0f, 0 },
    { "Full Volume", 2, 0.5f, 0.0f, 0.5f, 0.7f, 0.3f, 0, 1.0f, 0.0f, 0 }
};

#define NUM_FACTORY_PRESETS 128

EMSCRIPTEN_KEEPALIVE
int regroove_synth_get_preset_count() {
    return NUM_FACTORY_PRESETS;
}

EMSCRIPTEN_KEEPALIVE
const char* regroove_synth_get_preset_name(int index) {
    if (index < 0 || index >= NUM_FACTORY_PRESETS) return "";
    return factoryPresets[index].name;
}

EMSCRIPTEN_KEEPALIVE
void regroove_synth_load_preset(SIDSynthInstance* synth, int index, int voice) {
    if (!synth || !synth->sid) return;
    if (index < 0 || index >= NUM_FACTORY_PRESETS) return;
    if (voice < 0 || voice > 2) voice = 0;  // Default to Voice 1

    const WebPreset* preset = &factoryPresets[index];

    // Calculate parameter base for selected voice
    // Voice 0: params 0-7, Voice 1: params 8-15, Voice 2: params 16-23
    int base = voice * 8;

    // Apply to selected voice
    regroove_synth_set_parameter(synth, base + 0, (float)preset->waveform);
    regroove_synth_set_parameter(synth, base + 1, preset->pulseWidth);
    regroove_synth_set_parameter(synth, base + 2, preset->attack);
    regroove_synth_set_parameter(synth, base + 3, preset->decay);
    regroove_synth_set_parameter(synth, base + 4, preset->sustain);
    regroove_synth_set_parameter(synth, base + 5, preset->release);
    regroove_synth_set_parameter(synth, base + 6, 0.0f);  // Ring mod off
    regroove_synth_set_parameter(synth, base + 7, 0.0f);  // Sync off

    // Special presets with sync/ring
    if (index == 11 || index == 24 || index == 25) {  // Sync presets
        regroove_synth_set_parameter(synth, base + 7, 1.0f);  // Sync on
    }
    if (index == 29 || index == 68) {  // Ring presets
        regroove_synth_set_parameter(synth, base + 6, 1.0f);  // Ring mod on
    }

    // Filter (params 24-29) - GLOBAL, only update when loading to Voice 1
    if (voice == 0) {
        regroove_synth_set_parameter(synth, 24, (float)preset->filterMode);
        regroove_synth_set_parameter(synth, 25, preset->filterCutoff);
        regroove_synth_set_parameter(synth, 26, preset->filterResonance);
    }

    // Filter routing: param 27=V1, 28=V2, 29=V3
    regroove_synth_set_parameter(synth, 27 + voice, preset->filterVoice1 ? 1.0f : 0.0f);

    // Volume (param 30) - GLOBAL, only update when loading to Voice 1
    if (voice == 0) {
        regroove_synth_set_parameter(synth, 30, 0.7f);
    }
}
