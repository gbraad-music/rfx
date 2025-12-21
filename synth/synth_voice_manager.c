/*
 * Regroove Voice Manager Implementation
 */

#include "synth_voice_manager.h"
#include <stdlib.h>
#include <string.h>

struct SynthVoiceManager {
    int max_voices;
    VoiceMeta voices[MAX_POLYPHONY];
    int global_age;  // Incrementing counter for voice age
};

SynthVoiceManager* synth_voice_manager_create(int max_voices)
{
    if (max_voices <= 0 || max_voices > MAX_POLYPHONY) {
        return NULL;
    }

    SynthVoiceManager* vm = (SynthVoiceManager*)malloc(sizeof(SynthVoiceManager));
    if (!vm) return NULL;

    vm->max_voices = max_voices;
    vm->global_age = 0;

    for (int i = 0; i < MAX_POLYPHONY; i++) {
        vm->voices[i].state = VOICE_INACTIVE;
        vm->voices[i].note = 0;
        vm->voices[i].velocity = 0;
        vm->voices[i].age = 0;
        vm->voices[i].amplitude = 0.0f;
    }

    return vm;
}

void synth_voice_manager_destroy(SynthVoiceManager* vm)
{
    if (vm) free(vm);
}

void synth_voice_manager_reset(SynthVoiceManager* vm)
{
    if (!vm) return;

    for (int i = 0; i < vm->max_voices; i++) {
        vm->voices[i].state = VOICE_INACTIVE;
        vm->voices[i].note = 0;
        vm->voices[i].velocity = 0;
        vm->voices[i].age = 0;
        vm->voices[i].amplitude = 0.0f;
    }
    vm->global_age = 0;
}

int synth_voice_manager_allocate(SynthVoiceManager* vm, uint8_t note, uint8_t velocity)
{
    if (!vm) return -1;

    // First, check if this note is already playing (retrigger)
    for (int i = 0; i < vm->max_voices; i++) {
        if (vm->voices[i].state != VOICE_INACTIVE && vm->voices[i].note == note) {
            // Retrigger this voice
            vm->voices[i].state = VOICE_ACTIVE;
            vm->voices[i].velocity = velocity;
            vm->voices[i].age = vm->global_age++;
            vm->voices[i].amplitude = 1.0f;
            return i;
        }
    }

    // Look for an inactive voice
    for (int i = 0; i < vm->max_voices; i++) {
        if (vm->voices[i].state == VOICE_INACTIVE) {
            vm->voices[i].state = VOICE_ACTIVE;
            vm->voices[i].note = note;
            vm->voices[i].velocity = velocity;
            vm->voices[i].age = vm->global_age++;
            vm->voices[i].amplitude = 1.0f;
            return i;
        }
    }

    // No free voices - need to steal one
    // Strategy: steal oldest releasing voice first, then oldest active voice
    int steal_index = -1;
    int oldest_age = vm->global_age;

    // First pass: look for releasing voices
    for (int i = 0; i < vm->max_voices; i++) {
        if (vm->voices[i].state == VOICE_RELEASING && vm->voices[i].age < oldest_age) {
            oldest_age = vm->voices[i].age;
            steal_index = i;
        }
    }

    // Second pass: if no releasing voices, steal oldest active voice
    if (steal_index == -1) {
        oldest_age = vm->global_age;
        for (int i = 0; i < vm->max_voices; i++) {
            if (vm->voices[i].state == VOICE_ACTIVE && vm->voices[i].age < oldest_age) {
                oldest_age = vm->voices[i].age;
                steal_index = i;
            }
        }
    }

    // Steal the voice
    if (steal_index != -1) {
        vm->voices[steal_index].state = VOICE_ACTIVE;
        vm->voices[steal_index].note = note;
        vm->voices[steal_index].velocity = velocity;
        vm->voices[steal_index].age = vm->global_age++;
        vm->voices[steal_index].amplitude = 1.0f;
        return steal_index;
    }

    return -1;
}

int synth_voice_manager_release(SynthVoiceManager* vm, uint8_t note)
{
    if (!vm) return -1;

    // Find the voice playing this note
    for (int i = 0; i < vm->max_voices; i++) {
        if (vm->voices[i].state == VOICE_ACTIVE && vm->voices[i].note == note) {
            vm->voices[i].state = VOICE_RELEASING;
            return i;
        }
    }

    return -1;
}

void synth_voice_manager_stop_voice(SynthVoiceManager* vm, int voice_index)
{
    if (!vm || voice_index < 0 || voice_index >= vm->max_voices) return;

    vm->voices[voice_index].state = VOICE_INACTIVE;
    vm->voices[voice_index].note = 0;
    vm->voices[voice_index].velocity = 0;
    vm->voices[voice_index].amplitude = 0.0f;
}

void synth_voice_manager_update_amplitude(SynthVoiceManager* vm, int voice_index, float amplitude)
{
    if (!vm || voice_index < 0 || voice_index >= vm->max_voices) return;

    vm->voices[voice_index].amplitude = amplitude;
}

const VoiceMeta* synth_voice_manager_get_voice(SynthVoiceManager* vm, int voice_index)
{
    if (!vm || voice_index < 0 || voice_index >= vm->max_voices) return NULL;

    return &vm->voices[voice_index];
}

int synth_voice_manager_get_max_voices(SynthVoiceManager* vm)
{
    if (!vm) return 0;
    return vm->max_voices;
}

void synth_voice_manager_all_notes_off(SynthVoiceManager* vm)
{
    if (!vm) return;

    for (int i = 0; i < vm->max_voices; i++) {
        if (vm->voices[i].state == VOICE_ACTIVE) {
            vm->voices[i].state = VOICE_RELEASING;
        }
    }
}
