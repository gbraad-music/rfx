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
    if (!synth) return 0.0f;

    // Read from first voice (all voices have same params)
    AhxInstrumentParams* params = &synth->voices[0].params;

    switch (index) {
        // Oscillator
        case 0: return (float)params->waveform;
        case 1: return (float)params->wave_length;
        case 2: return (float)params->volume;

        // Envelope
        case 3: return (float)params->envelope.attack_frames;
        case 4: return (float)params->envelope.attack_volume;
        case 5: return (float)params->envelope.decay_frames;
        case 6: return (float)params->envelope.decay_volume;
        case 7: return (float)params->envelope.sustain_frames;
        case 8: return (float)params->envelope.release_frames;
        case 9: return (float)params->envelope.release_volume;

        // Filter
        case 10: return (float)params->filter_lower;
        case 11: return (float)params->filter_upper;
        case 12: return (float)params->filter_speed;
        case 13: return params->filter_enabled ? 1.0f : 0.0f;

        // Square/PWM
        case 14: return (float)params->square_lower;
        case 15: return (float)params->square_upper;
        case 16: return (float)params->square_speed;
        case 17: return params->square_enabled ? 1.0f : 0.0f;

        // Vibrato
        case 18: return (float)params->vibrato_delay;
        case 19: return (float)params->vibrato_depth;
        case 20: return (float)params->vibrato_speed;

        // Hard cut
        case 21: return params->hard_cut_release ? 1.0f : 0.0f;
        case 22: return (float)params->hard_cut_frames;

        default: return 0.0f;
    }
}

EMSCRIPTEN_KEEPALIVE
void regroove_synth_set_parameter(AhxSynthInstance* synth, int index, float value) {
    if (!synth) return;

    // Update all voices with new parameters
    // Parameter indices match plugin DistrhoPluginInfo.h exactly
    for (int i = 0; i < MAX_VOICES; i++) {
        AhxInstrumentParams* params = &synth->voices[i].params;
        bool recalc_adsr = false;
        bool regen_waveform = false;

        switch (index) {
            // Oscillator Group
            case 0:  // kParameterWaveform
                params->waveform = (uint8_t)value;
                synth->voices[i].core_inst.Waveform = (uint8_t)value;
                synth->voices[i].voice.Waveform = (uint8_t)value;
                regen_waveform = true;
                break;
            case 1:  // kParameterWaveLength
                params->wave_length = (uint8_t)value;
                synth->voices[i].core_inst.WaveLength = (uint8_t)value;
                synth->voices[i].voice.WaveLength = (uint8_t)value;
                regen_waveform = true;
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

        // Regenerate waveform if waveform or wave_length changed
        if (regen_waveform) {
            ahx_synth_generate_waveform(&synth->voices[i].voice,
                synth->voices[i].voice.Waveform,
                synth->voices[i].voice.WaveLength);
            // Update voice playback with new waveform buffer
            tracker_voice_set_waveform_16bit(&synth->voices[i].voice.voice_playback,
                synth->voices[i].voice.VoiceBuffer, 0x280);
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

// ==============================================================================
// Preset Management Functions
// ==============================================================================

#include "synth/ahx_preset.h"
#include <stdio.h>

// Get PList length for current preset
EMSCRIPTEN_KEEPALIVE
int regroove_synth_get_plist_length(AhxSynthInstance* synth) {
    if (!synth || !synth->voices[0].params.plist) return 0;
    return synth->voices[0].params.plist->length;
}

// Get PList speed for current preset
EMSCRIPTEN_KEEPALIVE
int regroove_synth_get_plist_speed(AhxSynthInstance* synth) {
    if (!synth || !synth->voices[0].params.plist) return 6;
    return synth->voices[0].params.plist->speed;
}

// Set PList speed
EMSCRIPTEN_KEEPALIVE
void regroove_synth_set_plist_speed(AhxSynthInstance* synth, int speed) {
    if (!synth) return;

    // Update all voices
    for (int i = 0; i < MAX_VOICES; i++) {
        if (synth->voices[i].params.plist) {
            synth->voices[i].params.plist->speed = speed;
        }
    }
}

// Get PList entry data (returns pointer to entry for JS to read)
EMSCRIPTEN_KEEPALIVE
AhxPListEntry* regroove_synth_get_plist_entry(AhxSynthInstance* synth, int index) {
    if (!synth || !synth->voices[0].params.plist) return NULL;
    if (index < 0 || index >= synth->voices[0].params.plist->length) return NULL;
    return &synth->voices[0].params.plist->entries[index];
}

// Set PList entry
EMSCRIPTEN_KEEPALIVE
void regroove_synth_set_plist_entry(AhxSynthInstance* synth, int index,
                                    uint8_t note, uint8_t fixed, uint8_t waveform,
                                    uint8_t fx0, uint8_t fx0_param, uint8_t fx1, uint8_t fx1_param) {
    if (!synth) return;

    // Update all voices
    for (int v = 0; v < MAX_VOICES; v++) {
        if (synth->voices[v].params.plist && index >= 0 && index < synth->voices[v].params.plist->length) {
            AhxPListEntry* e = &synth->voices[v].params.plist->entries[index];
            e->note = note;
            e->fixed = fixed != 0;
            e->waveform = waveform;
            e->fx[0] = fx0;
            e->fx_param[0] = fx0_param;
            e->fx[1] = fx1;
            e->fx_param[1] = fx1_param;
        }
    }
}

// Add PList entry
EMSCRIPTEN_KEEPALIVE
void regroove_synth_add_plist_entry(AhxSynthInstance* synth) {
    if (!synth) return;

    for (int v = 0; v < MAX_VOICES; v++) {
        AhxInstrumentParams* params = &synth->voices[v].params;

        // Create PList if it doesn't exist
        if (!params->plist) {
            params->plist = (AhxPList*)malloc(sizeof(AhxPList));
            params->plist->speed = 6;
            params->plist->length = 0;
            params->plist->entries = NULL;
        }

        // Reallocate entries array
        int new_length = params->plist->length + 1;
        AhxPListEntry* new_entries = (AhxPListEntry*)realloc(params->plist->entries,
                                                              new_length * sizeof(AhxPListEntry));
        if (new_entries) {
            params->plist->entries = new_entries;
            // Initialize new entry
            memset(&params->plist->entries[params->plist->length], 0, sizeof(AhxPListEntry));
            params->plist->length = new_length;
        }
    }
}

// Remove last PList entry
EMSCRIPTEN_KEEPALIVE
void regroove_synth_remove_plist_entry(AhxSynthInstance* synth) {
    if (!synth) return;

    for (int v = 0; v < MAX_VOICES; v++) {
        if (synth->voices[v].params.plist && synth->voices[v].params.plist->length > 0) {
            synth->voices[v].params.plist->length--;
        }
    }
}

// Clear all PList entries
EMSCRIPTEN_KEEPALIVE
void regroove_synth_clear_plist(AhxSynthInstance* synth) {
    if (!synth) return;

    for (int v = 0; v < MAX_VOICES; v++) {
        if (synth->voices[v].params.plist) {
            if (synth->voices[v].params.plist->entries) {
                free(synth->voices[v].params.plist->entries);
            }
            free(synth->voices[v].params.plist);
            synth->voices[v].params.plist = NULL;
        }
    }
}

// Export current preset to .ahxp format in memory (returns malloc'd buffer)
EMSCRIPTEN_KEEPALIVE
uint8_t* regroove_synth_export_preset(AhxSynthInstance* synth, const char* name, int* out_size) {
    if (!synth || !out_size) return NULL;

    // Create preset from first voice
    AhxPreset preset;
    strncpy(preset.name, name, 63);
    preset.name[63] = '\0';
    strcpy(preset.description, "Exported from web synth");
    memcpy(&preset.params, &synth->voices[0].params, sizeof(AhxInstrumentParams));

    // Calculate total size: AhxPreset + PList data (if present)
    size_t total_size = sizeof(AhxPreset);
    AhxPList* plist = synth->voices[0].params.plist;

    if (plist && plist->length > 0) {
        // PList format: speed (1 byte) + length (1 byte) + entries (7 bytes each)
        total_size += 2 + (plist->length * 7);
    }

    // Allocate buffer
    *out_size = total_size;
    uint8_t* buffer = (uint8_t*)malloc(total_size);
    if (!buffer) return NULL;

    // Write AhxPreset struct
    memcpy(buffer, &preset, sizeof(AhxPreset));
    size_t offset = sizeof(AhxPreset);

    // Write PList data if present
    if (plist && plist->length > 0) {
        emscripten_log(EM_LOG_CONSOLE, "[Export] Writing PList: %d entries at speed %d",
            plist->length, plist->speed);

        buffer[offset++] = plist->speed;
        buffer[offset++] = plist->length;

        for (int i = 0; i < plist->length; i++) {
            AhxPListEntry* e = &plist->entries[i];
            buffer[offset++] = e->note;
            buffer[offset++] = e->fixed ? 1 : 0;
            buffer[offset++] = e->waveform;
            buffer[offset++] = e->fx[0];
            buffer[offset++] = e->fx_param[0];
            buffer[offset++] = e->fx[1];
            buffer[offset++] = e->fx_param[1];

            emscripten_log(EM_LOG_CONSOLE, "[Export] PList[%d]: note=%d, fixed=%d, waveform=%d, fx0=%d(0x%02x), fx1=%d(0x%02x)",
                i, e->note, e->fixed, e->waveform, e->fx[0], e->fx_param[0], e->fx[1], e->fx_param[1]);
        }
    }

    emscripten_log(EM_LOG_CONSOLE, "[Export] Total size: %zu bytes (AhxPreset=%zu, PList=%zu)",
        total_size, sizeof(AhxPreset), total_size - sizeof(AhxPreset));

    return buffer;
}

// Free exported preset buffer
EMSCRIPTEN_KEEPALIVE
void regroove_synth_free_preset_buffer(uint8_t* buffer) {
    free(buffer);
}

// Import preset from binary buffer (.ahxp file format with 16-byte header)
EMSCRIPTEN_KEEPALIVE
int regroove_synth_import_preset(AhxSynthInstance* synth, const uint8_t* buffer, int size) {
    if (!synth || !buffer || size < 16 + sizeof(AhxPreset)) {
        return 0; // Fail
    }

    // Verify magic header "AHXP"
    if (buffer[0] != 'A' || buffer[1] != 'H' || buffer[2] != 'X' || buffer[3] != 'P') {
        emscripten_log(EM_LOG_ERROR, "[Import] Invalid .ahxp file - wrong magic");
        return 0;
    }

    // Skip 16-byte header and read preset fields (fixed format: name+author+desc+params = 64+64+256+32 = 416 bytes)
    AhxPreset preset;
    size_t offset = 16;

    // Read name (64 bytes)
    memcpy(preset.name, buffer + offset, 64);
    offset += 64;

    // Read author (64 bytes)
    memcpy(preset.author, buffer + offset, 64);
    offset += 64;

    // Read description (256 bytes)
    memcpy(preset.description, buffer + offset, 256);
    offset += 256;

    // Read packed parameters (32 bytes)
    const uint8_t* params = buffer + offset;
    preset.params.waveform = (AhxWaveform)params[0];
    preset.params.wave_length = params[1];
    preset.params.volume = params[2];
    preset.params.envelope.attack_frames = params[3];
    preset.params.envelope.attack_volume = params[4];
    preset.params.envelope.decay_frames = params[5];
    preset.params.envelope.decay_volume = params[6];
    preset.params.envelope.sustain_frames = params[7];
    preset.params.envelope.release_frames = params[8];
    preset.params.envelope.release_volume = params[9];
    preset.params.filter_lower = params[10];
    preset.params.filter_upper = params[11];
    preset.params.filter_speed = params[12];
    preset.params.filter_enabled = params[13] != 0;
    preset.params.square_lower = params[14];
    preset.params.square_upper = params[15];
    preset.params.square_speed = params[16];
    preset.params.square_enabled = params[17] != 0;
    preset.params.vibrato_delay = params[18];
    preset.params.vibrato_depth = params[19];
    preset.params.vibrato_speed = params[20];
    preset.params.hard_cut_release = params[21] != 0;
    preset.params.hard_cut_frames = params[22];
    preset.params.plist = NULL;
    offset += 32;

    // Apply to all voices
    for (int v = 0; v < MAX_VOICES; v++) {
        // Copy parameters (this does a shallow copy, which is okay for most fields)
        memcpy(&synth->voices[v].params, &preset.params, sizeof(AhxInstrumentParams));

        // Deep copy PList if present
        if (preset.params.plist) {
            if (synth->voices[v].params.plist) {
                if (synth->voices[v].params.plist->entries) {
                    free(synth->voices[v].params.plist->entries);
                }
                free(synth->voices[v].params.plist);
            }

            synth->voices[v].params.plist = (AhxPList*)malloc(sizeof(AhxPList));
            synth->voices[v].params.plist->speed = preset.params.plist->speed;
            synth->voices[v].params.plist->length = preset.params.plist->length;

            if (preset.params.plist->length > 0 && preset.params.plist->entries) {
                size_t entries_size = preset.params.plist->length * sizeof(AhxPListEntry);
                synth->voices[v].params.plist->entries = (AhxPListEntry*)malloc(entries_size);
                memcpy(synth->voices[v].params.plist->entries, preset.params.plist->entries, entries_size);
            } else {
                synth->voices[v].params.plist->entries = NULL;
            }
        } else {
            synth->voices[v].params.plist = NULL;
        }

        // Update core instrument parameters
        synth->voices[v].core_inst.Waveform = preset.params.waveform;
        synth->voices[v].core_inst.WaveLength = preset.params.wave_length;
        synth->voices[v].core_inst.Volume = preset.params.volume;
        synth->voices[v].core_inst.FilterLowerLimit = preset.params.filter_lower;
        synth->voices[v].core_inst.FilterUpperLimit = preset.params.filter_upper;
        synth->voices[v].core_inst.FilterSpeed = preset.params.filter_speed;
        synth->voices[v].core_inst.SquareLowerLimit = preset.params.square_lower;
        synth->voices[v].core_inst.SquareUpperLimit = preset.params.square_upper;
        synth->voices[v].core_inst.SquareSpeed = preset.params.square_speed;
        synth->voices[v].core_inst.VibratoDelay = preset.params.vibrato_delay;
        synth->voices[v].core_inst.VibratoDepth = preset.params.vibrato_depth;
        synth->voices[v].core_inst.VibratoSpeed = preset.params.vibrato_speed;
        synth->voices[v].core_inst.HardCutRelease = preset.params.hard_cut_release ? 1 : 0;
        synth->voices[v].core_inst.HardCutReleaseFrames = preset.params.hard_cut_frames;

        // Recalculate ADSR
        synth->voices[v].core_inst.Envelope.aFrames = preset.params.envelope.attack_frames;
        synth->voices[v].core_inst.Envelope.aVolume = preset.params.envelope.attack_volume;
        synth->voices[v].core_inst.Envelope.dFrames = preset.params.envelope.decay_frames;
        synth->voices[v].core_inst.Envelope.dVolume = preset.params.envelope.decay_volume;
        synth->voices[v].core_inst.Envelope.sFrames = preset.params.envelope.sustain_frames;
        synth->voices[v].core_inst.Envelope.rFrames = preset.params.envelope.release_frames;
        synth->voices[v].core_inst.Envelope.rVolume = preset.params.envelope.release_volume;

        emscripten_log(EM_LOG_CONSOLE, "[Import] Envelope set: a=%d(%d) d=%d(%d) s=%d r=%d(%d)",
            synth->voices[v].core_inst.Envelope.aFrames, synth->voices[v].core_inst.Envelope.aVolume,
            synth->voices[v].core_inst.Envelope.dFrames, synth->voices[v].core_inst.Envelope.dVolume,
            synth->voices[v].core_inst.Envelope.sFrames,
            synth->voices[v].core_inst.Envelope.rFrames, synth->voices[v].core_inst.Envelope.rVolume);

        ahx_synth_voice_calc_adsr(&synth->voices[v].voice, &synth->voices[v].core_inst);

        emscripten_log(EM_LOG_CONSOLE, "[Import] ADSR calculated: a=%d d=%d s=%d r=%d",
            synth->voices[v].voice.ADSR.aFrames, synth->voices[v].voice.ADSR.dFrames,
            synth->voices[v].voice.ADSR.sFrames, synth->voices[v].voice.ADSR.rFrames);

        // If voice is active, regenerate waveform immediately
        // Otherwise it will use old waveform buffer until next note_on
        if (synth->voices[v].voice.TrackOn) {
            synth->voices[v].voice.Waveform = preset.params.waveform;
            synth->voices[v].voice.WaveLength = preset.params.wave_length;
            synth->voices[v].voice.NewWaveform = 1;
        }
    }

    // Parse PList data if present (comes after fixed-size preset data at offset 432)
    size_t plist_offset = 432;  // 16 (header) + 416 (name+author+desc+params)
    emscripten_log(EM_LOG_CONSOLE, "[Import DEBUG] Buffer size=%d, plist_offset=%zu",
        size, plist_offset);

    if (size > plist_offset + 2) {
        uint8_t plist_speed = buffer[plist_offset];
        uint8_t plist_length = buffer[plist_offset + 1];
        emscripten_log(EM_LOG_CONSOLE, "[Import DEBUG] Found PList header: speed=%d, length=%d, required_size=%zu",
            plist_speed, plist_length, plist_offset + 2 + (plist_length * 7));

        if (plist_length > 0 && size >= plist_offset + 2 + (plist_length * 7)) {
            emscripten_log(EM_LOG_CONSOLE, "[Import] PList data found: %d entries at speed %d", plist_length, plist_speed);

            // Clear existing PList (operates on all voices)
            regroove_synth_clear_plist(synth);

            // CRITICAL: Create PList with correct speed for ALL voices BEFORE adding entries
            // Otherwise add_plist_entry() will create them with default speed=6!
            for (int v = 0; v < MAX_VOICES; v++) {
                if (!synth->voices[v].params.plist) {
                    synth->voices[v].params.plist = (AhxPList*)malloc(sizeof(AhxPList));
                    synth->voices[v].params.plist->speed = plist_speed;
                    synth->voices[v].params.plist->length = 0;
                    synth->voices[v].params.plist->entries = NULL;
                    emscripten_log(EM_LOG_CONSOLE, "[Import] Created PList for voice %d with speed=%d", v, plist_speed);
                }
            }

            // Parse and add entries (this adds to ALL voices)
            size_t entry_offset = plist_offset + 2;
            for (int i = 0; i < plist_length; i++) {
                regroove_synth_add_plist_entry(synth);

                uint8_t note = buffer[entry_offset++];
                uint8_t fixed = buffer[entry_offset++];
                uint8_t waveform = buffer[entry_offset++];
                uint8_t fx0 = buffer[entry_offset++];
                uint8_t fx0_param = buffer[entry_offset++];
                uint8_t fx1 = buffer[entry_offset++];
                uint8_t fx1_param = buffer[entry_offset++];

                regroove_synth_set_plist_entry(synth, i, note, fixed, waveform, fx0, fx0_param, fx1, fx1_param);

                emscripten_log(EM_LOG_CONSOLE, "[Import] PList[%d]: note=%d, fixed=%d, waveform=%d, fx0=%d(0x%02x), fx1=%d(0x%02x)",
                    i, note, fixed, waveform, fx0, fx0_param, fx1, fx1_param);
            }

            // CRITICAL: Now divide ADSR/Filter/Square speeds by PList speed to convert ticks to frames!
            if (plist_speed > 0) {
                for (int v = 0; v < MAX_VOICES; v++) {
                    emscripten_log(EM_LOG_CONSOLE, "[Import] BEFORE speed division: ADSR a=%d d=%d s=%d r=%d, Filter=%d, Square=%d",
                        synth->voices[v].core_inst.Envelope.aFrames,
                        synth->voices[v].core_inst.Envelope.dFrames,
                        synth->voices[v].core_inst.Envelope.sFrames,
                        synth->voices[v].core_inst.Envelope.rFrames,
                        synth->voices[v].core_inst.FilterSpeed,
                        synth->voices[v].core_inst.SquareSpeed);

                    // Convert CIA ticks to 50Hz frames - ALL timing parameters!
                    synth->voices[v].core_inst.Envelope.aFrames = (synth->voices[v].core_inst.Envelope.aFrames + plist_speed - 1) / plist_speed;
                    synth->voices[v].core_inst.Envelope.dFrames = (synth->voices[v].core_inst.Envelope.dFrames + plist_speed - 1) / plist_speed;
                    synth->voices[v].core_inst.Envelope.sFrames = (synth->voices[v].core_inst.Envelope.sFrames + plist_speed - 1) / plist_speed;
                    synth->voices[v].core_inst.Envelope.rFrames = (synth->voices[v].core_inst.Envelope.rFrames + plist_speed - 1) / plist_speed;

                    // CRITICAL: Filter and Square speeds are ALSO in ticks!
                    synth->voices[v].core_inst.FilterSpeed = (synth->voices[v].core_inst.FilterSpeed + plist_speed - 1) / plist_speed;
                    synth->voices[v].core_inst.SquareSpeed = (synth->voices[v].core_inst.SquareSpeed + plist_speed - 1) / plist_speed;

                    emscripten_log(EM_LOG_CONSOLE, "[Import] AFTER speed division by %d: ADSR a=%d d=%d s=%d r=%d, Filter=%d, Square=%d",
                        plist_speed,
                        synth->voices[v].core_inst.Envelope.aFrames,
                        synth->voices[v].core_inst.Envelope.dFrames,
                        synth->voices[v].core_inst.Envelope.sFrames,
                        synth->voices[v].core_inst.Envelope.rFrames,
                        synth->voices[v].core_inst.FilterSpeed,
                        synth->voices[v].core_inst.SquareSpeed);

                    // Recalculate ADSR with corrected frame values
                    ahx_synth_voice_calc_adsr(&synth->voices[v].voice, &synth->voices[v].core_inst);

                    emscripten_log(EM_LOG_CONSOLE, "[Import] Final ADSR runtime: a=%d d=%d s=%d r=%d",
                        synth->voices[v].voice.ADSR.aFrames,
                        synth->voices[v].voice.ADSR.dFrames,
                        synth->voices[v].voice.ADSR.sFrames,
                        synth->voices[v].voice.ADSR.rFrames);
                }
            }
        } else {
            emscripten_log(EM_LOG_CONSOLE, "[Import DEBUG] PList data incomplete: length=%d but buffer too small", plist_length);
        }
    } else {
        emscripten_log(EM_LOG_CONSOLE, "[Import DEBUG] No PList data - buffer too small (size=%d, needed=%zu)", size, plist_offset + 2);
    }

    return 1; // Success
}
