/*
 * Regroove Synthesizer Voice Manager
 * Manages MIDI note events and voice allocation
 */

#ifndef SYNTH_VOICE_H
#define SYNTH_VOICE_H

#include "synth_common.h"
#include "synth_oscillator.h"
#include "synth_envelope.h"
#include "synth_filter.h"

#ifdef __cplusplus
extern "C" {
#endif

#define SYNTH_MAX_POLYPHONY 16

typedef struct {
    int note;
    int velocity;
    int active;

    // DSP components
    SynthOscillator* osc;
    SynthEnvelope* amp_env;
    SynthEnvelope* filter_env;
    SynthFilter* filter;
} SynthVoiceState;

typedef struct SynthVoiceManager SynthVoiceManager;

// Lifecycle
SynthVoiceManager* synth_voice_manager_create(int max_voices);
void synth_voice_manager_destroy(SynthVoiceManager* vm);
void synth_voice_manager_reset(SynthVoiceManager* vm);

// MIDI event handling
void synth_voice_manager_note_on(SynthVoiceManager* vm, int note, int velocity);
void synth_voice_manager_note_off(SynthVoiceManager* vm, int note);
void synth_voice_manager_all_notes_off(SynthVoiceManager* vm);

// Voice configuration
void synth_voice_manager_set_waveform(SynthVoiceManager* vm, SynthOscWaveform waveform);
void synth_voice_manager_set_filter_type(SynthVoiceManager* vm, SynthFilterType type);

// Get voice states for rendering
int synth_voice_manager_get_active_count(SynthVoiceManager* vm);
SynthVoiceState* synth_voice_manager_get_voice(SynthVoiceManager* vm, int index);

#ifdef __cplusplus
}
#endif

#endif // SYNTH_VOICE_H
