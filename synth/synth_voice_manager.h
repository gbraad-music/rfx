/*
 * Regroove Voice Manager for Polyphonic Synthesizers
 * Handles voice allocation, stealing, and note tracking
 */

#ifndef SYNTH_VOICE_MANAGER_H
#define SYNTH_VOICE_MANAGER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

#define MAX_POLYPHONY 16

typedef enum {
    VOICE_INACTIVE = 0,
    VOICE_ACTIVE,
    VOICE_RELEASING
} VoiceState;

typedef struct {
    VoiceState state;
    uint8_t note;
    uint8_t velocity;
    int age;              // For voice stealing (oldest first)
    float amplitude;      // For voice stealing (quietest first)
} VoiceMeta;

typedef struct SynthVoiceManager SynthVoiceManager;

/**
 * Create a voice manager
 * @param max_voices Maximum number of voices (polyphony)
 */
SynthVoiceManager* synth_voice_manager_create(int max_voices);

/**
 * Destroy a voice manager
 */
void synth_voice_manager_destroy(SynthVoiceManager* vm);

/**
 * Reset all voices to inactive
 */
void synth_voice_manager_reset(SynthVoiceManager* vm);

/**
 * Allocate a voice for a new note
 * Returns voice index, or -1 if none available
 * Will steal voices if necessary (oldest/quietest first)
 */
int synth_voice_manager_allocate(SynthVoiceManager* vm, uint8_t note, uint8_t velocity);

/**
 * Release a voice (note off)
 * Returns voice index, or -1 if note not found
 */
int synth_voice_manager_release(SynthVoiceManager* vm, uint8_t note);

/**
 * Mark a voice as completely inactive (envelope finished)
 */
void synth_voice_manager_stop_voice(SynthVoiceManager* vm, int voice_index);

/**
 * Update voice amplitude (for voice stealing decisions)
 */
void synth_voice_manager_update_amplitude(SynthVoiceManager* vm, int voice_index, float amplitude);

/**
 * Get voice metadata
 */
const VoiceMeta* synth_voice_manager_get_voice(SynthVoiceManager* vm, int voice_index);

/**
 * Get maximum voices
 */
int synth_voice_manager_get_max_voices(SynthVoiceManager* vm);

/**
 * All notes off (panic)
 */
void synth_voice_manager_all_notes_off(SynthVoiceManager* vm);

#ifdef __cplusplus
}
#endif

#endif // SYNTH_VOICE_MANAGER_H
