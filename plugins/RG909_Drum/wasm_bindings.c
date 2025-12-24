/*
 * WebAssembly Bindings for RG909 Drum Synthesizer
 * Standalone implementation without DPF dependencies
 */

#include <emscripten.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "synth/synth_oscillator.h"
#include "synth/synth_noise.h"
#include "synth/synth_filter.h"
#include "synth/synth_envelope.h"
#include "synth/synth_voice_manager.h"

#define MAX_DRUM_VOICES 7

// MIDI Note mapping (GM Drum Map)
#define MIDI_NOTE_BD  36  // Bass Drum
#define MIDI_NOTE_SD  38  // Snare Drum
#define MIDI_NOTE_LT  41  // Low Tom
#define MIDI_NOTE_MT  47  // Mid Tom
#define MIDI_NOTE_HT  50  // High Tom
#define MIDI_NOTE_RS  37  // Rimshot
#define MIDI_NOTE_HC  39  // Hand Clap

typedef enum {
    DRUM_BD = 0,
    DRUM_SD,
    DRUM_LT,
    DRUM_MT,
    DRUM_HT,
    DRUM_RS,
    DRUM_HC
} DrumType;

typedef struct {
    DrumType type;
    SynthOscillator* osc1;
    SynthOscillator* osc2;
    SynthNoise* noise;
    SynthFilter* filter;
    SynthEnvelope* env;
    SynthEnvelope* pitch_env;
    int active;
    int clap_stage;
    float clap_timer;
    float base_freq;  // Store base frequency for pitch envelope
} DrumVoice;

typedef struct {
    SynthVoiceManager* voice_manager;
    DrumVoice voices[MAX_DRUM_VOICES];

    // Parameters
    float bd_level, bd_tune, bd_decay, bd_attack;
    float sd_level, sd_tone, sd_snappy, sd_tuning;
    float lt_level, lt_tuning, lt_decay;
    float mt_level, mt_tuning, mt_decay;
    float ht_level, ht_tuning, ht_decay;
    float rs_level, rs_tuning;
    float hc_level, hc_tone;
    float master_volume;
} RG909Synth;

EMSCRIPTEN_KEEPALIVE
RG909Synth* rg909_create(float sample_rate) {
    RG909Synth* synth = (RG909Synth*)malloc(sizeof(RG909Synth));
    if (!synth) return NULL;

    // Initialize defaults
    synth->bd_level = 0.8f;
    synth->bd_tune = 0.5f;
    synth->bd_decay = 0.5f;
    synth->bd_attack = 0.0f;
    synth->sd_level = 0.7f;
    synth->sd_tone = 0.5f;
    synth->sd_snappy = 0.5f;
    synth->sd_tuning = 0.5f;
    synth->lt_level = 0.7f;
    synth->lt_tuning = 0.5f;
    synth->lt_decay = 0.5f;
    synth->mt_level = 0.7f;
    synth->mt_tuning = 0.5f;
    synth->mt_decay = 0.5f;
    synth->ht_level = 0.7f;
    synth->ht_tuning = 0.5f;
    synth->ht_decay = 0.5f;
    synth->rs_level = 0.6f;
    synth->rs_tuning = 0.5f;
    synth->hc_level = 0.6f;
    synth->hc_tone = 0.5f;
    synth->master_volume = 0.6f;

    synth->voice_manager = synth_voice_manager_create(MAX_DRUM_VOICES);

    for (int i = 0; i < MAX_DRUM_VOICES; i++) {
        synth->voices[i].osc1 = synth_oscillator_create();
        synth->voices[i].osc2 = synth_oscillator_create();
        synth->voices[i].noise = synth_noise_create();
        synth->voices[i].filter = synth_filter_create();
        synth->voices[i].env = synth_envelope_create();
        synth->voices[i].pitch_env = synth_envelope_create();
        synth->voices[i].active = 0;
        synth->voices[i].clap_stage = 0;
        synth->voices[i].clap_timer = 0.0f;
    }

    return synth;
}

EMSCRIPTEN_KEEPALIVE
void rg909_destroy(RG909Synth* synth) {
    if (!synth) return;

    for (int i = 0; i < MAX_DRUM_VOICES; i++) {
        if (synth->voices[i].osc1) synth_oscillator_destroy(synth->voices[i].osc1);
        if (synth->voices[i].osc2) synth_oscillator_destroy(synth->voices[i].osc2);
        if (synth->voices[i].noise) synth_noise_destroy(synth->voices[i].noise);
        if (synth->voices[i].filter) synth_filter_destroy(synth->voices[i].filter);
        if (synth->voices[i].env) synth_envelope_destroy(synth->voices[i].env);
        if (synth->voices[i].pitch_env) synth_envelope_destroy(synth->voices[i].pitch_env);
    }

    if (synth->voice_manager) synth_voice_manager_destroy(synth->voice_manager);
    free(synth);
}

EMSCRIPTEN_KEEPALIVE
void rg909_reset(RG909Synth* synth) {
    if (!synth) return;

    for (int i = 0; i < MAX_DRUM_VOICES; i++) {
        synth->voices[i].active = 0;
    }

    synth_voice_manager_reset(synth->voice_manager);
}

EMSCRIPTEN_KEEPALIVE
void rg909_trigger_drum(RG909Synth* synth, uint8_t note, uint8_t velocity, float sample_rate) {
    if (!synth) return;

    DrumType type;
    switch (note) {
        case MIDI_NOTE_BD: type = DRUM_BD; break;
        case MIDI_NOTE_SD: type = DRUM_SD; break;
        case MIDI_NOTE_LT: type = DRUM_LT; break;
        case MIDI_NOTE_MT: type = DRUM_MT; break;
        case MIDI_NOTE_HT: type = DRUM_HT; break;
        case MIDI_NOTE_RS: type = DRUM_RS; break;
        case MIDI_NOTE_HC: type = DRUM_HC; break;
        default: return;
    }

    int voice_idx = synth_voice_manager_allocate(synth->voice_manager, note, velocity);
    if (voice_idx < 0) return;

    DrumVoice* v = &synth->voices[voice_idx];
    v->type = type;
    v->active = 1;

    float vel_factor = velocity / 127.0f;

    // Configure drum based on type (simplified version)
    switch (type) {
        case DRUM_BD: { // Bass Drum
            v->base_freq = 50.0f + (synth->bd_tune - 0.5f) * 40.0f;
            synth_oscillator_set_frequency(v->osc1, v->base_freq);
            synth_oscillator_set_waveform(v->osc1, SYNTH_OSC_SINE);

            float attack = synth->bd_attack * 0.01f;
            float decay = 0.2f + synth->bd_decay * 0.3f;
            synth_envelope_set_attack(v->env, attack);
            synth_envelope_set_decay(v->env, decay);
            synth_envelope_set_sustain(v->env, 0.0f);
            synth_envelope_set_release(v->env, 0.01f);
            synth_envelope_trigger(v->env);

            synth_envelope_set_attack(v->pitch_env, 0.001f);
            synth_envelope_set_decay(v->pitch_env, 0.05f);
            synth_envelope_set_sustain(v->pitch_env, 0.0f);
            synth_envelope_set_release(v->pitch_env, 0.01f);
            synth_envelope_trigger(v->pitch_env);
            break;
        }

        case DRUM_SD: { // Snare Drum
            float freq = 180.0f + (synth->sd_tuning - 0.5f) * 100.0f;
            synth_oscillator_set_frequency(v->osc1, freq);
            synth_oscillator_set_frequency(v->osc2, freq * 1.6f);
            synth_oscillator_set_waveform(v->osc1, SYNTH_OSC_TRIANGLE);
            synth_oscillator_set_waveform(v->osc2, SYNTH_OSC_TRIANGLE);

            synth_envelope_set_attack(v->env, 0.001f);
            synth_envelope_set_decay(v->env, 0.15f);
            synth_envelope_set_sustain(v->env, 0.0f);
            synth_envelope_set_release(v->env, 0.01f);
            synth_envelope_trigger(v->env);
            break;
        }

        default:
            // Other drums - simplified implementation
            synth_envelope_set_attack(v->env, 0.001f);
            synth_envelope_set_decay(v->env, 0.2f);
            synth_envelope_set_sustain(v->env, 0.0f);
            synth_envelope_set_release(v->env, 0.01f);
            synth_envelope_trigger(v->env);
            break;
    }
}

EMSCRIPTEN_KEEPALIVE
void rg909_process_f32(RG909Synth* synth, float* buffer, int frames, float sample_rate) {
    if (!synth || !buffer) return;

    // Zero buffer
    for (int i = 0; i < frames * 2; i++) {
        buffer[i] = 0.0f;
    }

    // Process each active voice
    for (int v = 0; v < MAX_DRUM_VOICES; v++) {
        if (!synth->voices[v].active) continue;

        DrumVoice* voice = &synth->voices[v];

        for (int i = 0; i < frames; i++) {
            float env_val = synth_envelope_process(voice->env, sample_rate);

            if (env_val <= 0.0001f) {
                voice->active = 0;
                break;
            }

            float sample = 0.0f;

            // Simplified drum synthesis
            if (voice->type == DRUM_BD) {
                float pitch_env = synth_envelope_process(voice->pitch_env, sample_rate);
                synth_oscillator_set_frequency(voice->osc1, voice->base_freq * (1.0f + pitch_env * 2.0f));

                sample = synth_oscillator_process(voice->osc1, sample_rate);
                sample *= env_val * synth->bd_level;
            } else if (voice->type == DRUM_SD) {
                float tone = synth_oscillator_process(voice->osc1, sample_rate);
                tone += synth_oscillator_process(voice->osc2, sample_rate);
                float noise = synth_noise_process(voice->noise);

                sample = tone * (1.0f - synth->sd_snappy) + noise * synth->sd_snappy;
                sample *= env_val * synth->sd_level;
            } else {
                sample = synth_oscillator_process(voice->osc1, sample_rate);
                sample *= env_val * 0.7f;
            }

            sample *= synth->master_volume;

            // Stereo output (interleaved)
            buffer[i * 2] += sample;
            buffer[i * 2 + 1] += sample;
        }
    }
}

EMSCRIPTEN_KEEPALIVE
void rg909_set_parameter(RG909Synth* synth, int param, float value) {
    if (!synth) return;

    // Parameter indices match DistrhoPluginInfo.h
    switch (param) {
        case 0: synth->bd_level = value; break;
        case 1: synth->bd_tune = value; break;
        case 2: synth->bd_decay = value; break;
        case 3: synth->bd_attack = value; break;
        // Add more as needed
        case 23: synth->master_volume = value; break;
    }
}

EMSCRIPTEN_KEEPALIVE
float rg909_get_parameter(RG909Synth* synth, int param) {
    if (!synth) return 0.0f;

    switch (param) {
        case 0: return synth->bd_level;
        case 1: return synth->bd_tune;
        case 2: return synth->bd_decay;
        case 3: return synth->bd_attack;
        case 23: return synth->master_volume;
        default: return 0.0f;
    }
}
