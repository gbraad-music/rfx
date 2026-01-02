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
#include "rg909_bd.h"
#include "rg909_sd.h"

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
    float noise_env;           // Exponential noise envelope (also used as phase accumulator)
    float noise_decay;         // Noise decay coefficient
    float decay_env;           // Exponential decay VCA
    float decay_coeff;         // Decay coefficient
    float phase_offset;        // Phase offset for sweep-shape to ensure positive start
    float tail_phase_offset;   // Phase offset for tail to start at zero crossing
    float tail_slow_offset;    // Phase offset for tail_slow to start at zero crossing (inverted)
} RG909DrumVoice;

typedef struct {
    SynthVoiceManager* voice_manager;
    RG909DrumVoice voices[RG909_MAX_VOICES];

    // Modular drum voices (used for BD and SD)
    RG909_BD bd;
    RG909_SD sd;

    // Parameters (0.0 - 1.0 range, except bd_level which can exceed 1.0)
    float bd_level, bd_tune, bd_decay, bd_attack;
    float sd_level, sd_tone, sd_snappy, sd_tuning;
    float sd_freq1, sd_freq2, sd_noise_level, sd_lp_noise_cutoff;
    float sd_res1_decay, sd_res2_decay, sd_noise_decay;
    float sd_res1_level, sd_res2_level;
    float sd_noise_attack, sd_noise_fade_time, sd_noise_fade_curve;
    float sd_tone_gain;
    float lt_level, lt_tuning, lt_decay;
    float mt_level, mt_tuning, mt_decay;
    float ht_level, ht_tuning, ht_decay;
    float rs_level, rs_tuning;
    float hc_level, hc_tone;
    float master_volume;

    // BD Sweep-Shape Parameters (user-adjustable)
    float bd_squiggly_end_ms;    // End of squiggly phase (ms)
    float bd_fast_end_ms;        // End of fast sweep phase (ms)
    float bd_slow_end_ms;        // End of slow sweep phase (ms)
    float bd_tail_slow_start_ms; // Start of tail slow-down (ms)
    float bd_squiggly_freq;      // Squiggly frequency (Hz)
    float bd_fast_freq;          // Fast sweep frequency (Hz)
    float bd_slow_freq;          // Slow sweep frequency (Hz)
    float bd_tail_freq;          // Tail frequency (Hz)
    float bd_tail_slow_freq;     // Tail slow frequency (Hz)
    float bd_fast_saw_pct;       // Fast SAW width (percentage, 0-100)
    float bd_slow_saw_pct;       // Slow SAW width (percentage, 0-100)
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
