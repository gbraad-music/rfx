/*
 * Regroove Synthesizer Voice Manager Implementation
 */

#include "synth_voice.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

// Forward declaration
static void cleanup_voice_manager(SynthVoiceManager* vm);

struct SynthVoiceManager {
    SynthVoiceState* voices;
    int max_voices;
    int voice_count;
    SynthOscWaveform waveform;
    SynthFilterType filter_type;
};

SynthVoiceManager* synth_voice_manager_create(int max_voices)
{
    if (max_voices < 1 || max_voices > SYNTH_MAX_POLYPHONY) {
        max_voices = 1; // Default to monophonic
    }

    SynthVoiceManager* vm = (SynthVoiceManager*)malloc(sizeof(SynthVoiceManager));
    if (!vm) return NULL;

    vm->voices = (SynthVoiceState*)calloc(max_voices, sizeof(SynthVoiceState));
    if (!vm->voices) {
        free(vm);
        return NULL;
    }

    vm->max_voices = max_voices;
    vm->voice_count = 0;
    vm->waveform = SYNTH_OSC_SAW;
    vm->filter_type = SYNTH_FILTER_LPF;

    // Initialize all voices
    for (int i = 0; i < max_voices; i++) {
        vm->voices[i].osc = synth_oscillator_create();
        vm->voices[i].amp_env = synth_envelope_create();
        vm->voices[i].filter_env = synth_envelope_create();
        vm->voices[i].filter = synth_filter_create();
        vm->voices[i].active = 0;
        vm->voices[i].note = -1;
        vm->voices[i].velocity = 0;

        if (!vm->voices[i].osc || !vm->voices[i].amp_env ||
            !vm->voices[i].filter_env || !vm->voices[i].filter) {
            cleanup_voice_manager(vm);
            return NULL;
        }
    }

    return vm;
}

static void cleanup_voice_manager(SynthVoiceManager* vm)
{
    if (!vm) return;

    if (vm->voices) {
        for (int i = 0; i < vm->max_voices; i++) {
            synth_oscillator_destroy(vm->voices[i].osc);
            synth_envelope_destroy(vm->voices[i].amp_env);
            synth_envelope_destroy(vm->voices[i].filter_env);
            synth_filter_destroy(vm->voices[i].filter);
        }
        free(vm->voices);
    }
    free(vm);
}

void synth_voice_manager_destroy(SynthVoiceManager* vm)
{
    cleanup_voice_manager(vm);
}

void synth_voice_manager_reset(SynthVoiceManager* vm)
{
    if (!vm) return;

    for (int i = 0; i < vm->max_voices; i++) {
        synth_oscillator_reset(vm->voices[i].osc);
        synth_envelope_reset(vm->voices[i].amp_env);
        synth_envelope_reset(vm->voices[i].filter_env);
        synth_filter_reset(vm->voices[i].filter);
        vm->voices[i].active = 0;
        vm->voices[i].note = -1;
    }
    vm->voice_count = 0;
}

void synth_voice_manager_note_on(SynthVoiceManager* vm, int note, int velocity)
{
    if (!vm || note < 0 || note > 127) return;

    // Find an available voice (or steal oldest)
    int voice_idx = -1;

    // First, try to find an inactive voice
    for (int i = 0; i < vm->max_voices; i++) {
        if (!vm->voices[i].active) {
            voice_idx = i;
            break;
        }
    }

    // If no inactive voice, steal the first one (simple voice stealing)
    if (voice_idx == -1) {
        voice_idx = 0;
    }

    SynthVoiceState* voice = &vm->voices[voice_idx];

    // Set up the voice
    voice->note = note;
    voice->velocity = velocity;
    voice->active = 1;

    // Configure oscillator
    float freq = synth_midi_to_freq(note);
    synth_oscillator_set_frequency(voice->osc, freq);
    synth_oscillator_set_waveform(voice->osc, vm->waveform);

    // Trigger envelopes
    synth_envelope_trigger(voice->amp_env);
    synth_envelope_trigger(voice->filter_env);

    // Update voice count
    vm->voice_count = 0;
    for (int i = 0; i < vm->max_voices; i++) {
        if (vm->voices[i].active) vm->voice_count++;
    }
}

void synth_voice_manager_note_off(SynthVoiceManager* vm, int note)
{
    if (!vm) return;

    // Find and release all voices with this note
    for (int i = 0; i < vm->max_voices; i++) {
        if (vm->voices[i].active && vm->voices[i].note == note) {
            synth_envelope_release(vm->voices[i].amp_env);
            synth_envelope_release(vm->voices[i].filter_env);

            // Mark as inactive when envelope finishes
            if (!synth_envelope_is_active(vm->voices[i].amp_env)) {
                vm->voices[i].active = 0;
                vm->voices[i].note = -1;
            }
        }
    }

    // Update voice count
    vm->voice_count = 0;
    for (int i = 0; i < vm->max_voices; i++) {
        if (vm->voices[i].active) vm->voice_count++;
    }
}

void synth_voice_manager_all_notes_off(SynthVoiceManager* vm)
{
    if (!vm) return;

    for (int i = 0; i < vm->max_voices; i++) {
        if (vm->voices[i].active) {
            synth_envelope_release(vm->voices[i].amp_env);
            synth_envelope_release(vm->voices[i].filter_env);
        }
    }
}

void synth_voice_manager_set_waveform(SynthVoiceManager* vm, SynthOscWaveform waveform)
{
    if (!vm) return;
    vm->waveform = waveform;
}

void synth_voice_manager_set_filter_type(SynthVoiceManager* vm, SynthFilterType type)
{
    if (!vm) return;
    vm->filter_type = type;

    for (int i = 0; i < vm->max_voices; i++) {
        synth_filter_set_type(vm->voices[i].filter, type);
    }
}

int synth_voice_manager_get_active_count(SynthVoiceManager* vm)
{
    return vm ? vm->voice_count : 0;
}

SynthVoiceState* synth_voice_manager_get_voice(SynthVoiceManager* vm, int index)
{
    if (!vm || index < 0 || index >= vm->max_voices) return NULL;
    return &vm->voices[index];
}
