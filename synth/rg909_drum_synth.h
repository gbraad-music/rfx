/*
 * RG909 Drum Synthesizer - Shared Core Implementation
 * TR-909 Style Drum Synthesis Engine
 */

#ifndef RG909_DRUM_SYNTH_H
#define RG909_DRUM_SYNTH_H

#include "synth_resonator.h"
#include "synth_noise.h"
#include "synth_filter.h"
#include "synth_envelope.h"
#include "synth_voice_manager.h"

#ifdef __cplusplus
extern "C" {
#endif

#define RG909_MAX_VOICES 7

// MIDI Note mapping (GM Drum Map)
#define RG909_MIDI_NOTE_BD  36  // Bass Drum
#define RG909_MIDI_NOTE_RS  37  // Rimshot
#define RG909_MIDI_NOTE_SD  38  // Snare Drum
#define RG909_MIDI_NOTE_HC  39  // Hand Clap
#define RG909_MIDI_NOTE_LT  41  // Low Tom
#define RG909_MIDI_NOTE_MT  47  // Mid Tom
#define RG909_MIDI_NOTE_HT  50  // High Tom

typedef enum {
    RG909_DRUM_BD = 0,  // Bass Drum
    RG909_DRUM_SD,      // Snare Drum
    RG909_DRUM_LT,      // Low Tom
    RG909_DRUM_MT,      // Mid Tom
    RG909_DRUM_HT,      // High Tom
    RG909_DRUM_RS,      // Rimshot
    RG909_DRUM_HC       // Hand Clap
} RG909DrumType;

typedef struct {
    RG909DrumType type;
    SynthResonator* res1;      // Primary resonator
    SynthResonator* res2;      // Secondary resonator (snare, clap)
    SynthNoise* noise;
    SynthFilter* filter;
    SynthEnvelope* env;        // For clap bursts only
    int active;
    int clap_stage;
    float clap_timer;
    float base_freq;
    float sweep_pos;           // Current position in pitch sweep / time counter
    float sweep_time;          // Total sweep duration
    float sweep_amount;        // Sweep multiplier
    float noise_env;           // Exponential noise envelope
    float noise_decay;         // Noise decay coefficient
    float decay_env;           // Exponential decay VCA
    float decay_coeff;         // Decay coefficient
} RG909DrumVoice;

typedef struct {
    SynthVoiceManager* voice_manager;
    RG909DrumVoice voices[RG909_MAX_VOICES];

    // Parameters (0.0 - 1.0 range)
    float bd_level, bd_tune, bd_decay, bd_attack;
    float sd_level, sd_tone, sd_snappy, sd_tuning;
    float lt_level, lt_tuning, lt_decay;
    float mt_level, mt_tuning, mt_decay;
    float ht_level, ht_tuning, ht_decay;
    float rs_level, rs_tuning;
    float hc_level, hc_tone;
    float master_volume;
} RG909Synth;

// Core API
RG909Synth* rg909_synth_create(void);
void rg909_synth_destroy(RG909Synth* synth);
void rg909_synth_reset(RG909Synth* synth);

// Trigger drums
void rg909_synth_trigger_drum(RG909Synth* synth, uint8_t note, uint8_t velocity, float sample_rate);

// Process audio (interleaved stereo)
void rg909_synth_process_interleaved(RG909Synth* synth, float* buffer, int frames, float sample_rate);

// Parameter control
void rg909_synth_set_parameter(RG909Synth* synth, int param_index, float value);
float rg909_synth_get_parameter(RG909Synth* synth, int param_index);

#ifdef __cplusplus
}
#endif

#endif // RG909_DRUM_SYNTH_H
