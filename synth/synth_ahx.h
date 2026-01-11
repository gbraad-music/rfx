/*
 * AHX-Style Synthesizer Engine
 * Amiga-style wavetable synth with ADSR, filter, and modulation
 */

#ifndef SYNTH_AHX_H
#define SYNTH_AHX_H

#include "synth_common.h"

#ifdef __cplusplus
extern "C" {
#endif

// Waveform types
typedef enum {
    SYNTH_AHX_WAVE_TRIANGLE = 0,
    SYNTH_AHX_WAVE_SAWTOOTH = 1,
    SYNTH_AHX_WAVE_SQUARE = 2,
    SYNTH_AHX_WAVE_NOISE = 3
} SynthAHXWaveform;

// Filter types
typedef enum {
    SYNTH_AHX_FILTER_NONE = 0,
    SYNTH_AHX_FILTER_LOWPASS = 1,
    SYNTH_AHX_FILTER_HIGHPASS = 2
} SynthAHXFilterType;

typedef struct SynthAHXVoice SynthAHXVoice;

// Voice lifecycle
SynthAHXVoice* synth_ahx_voice_create(void);
void synth_ahx_voice_destroy(SynthAHXVoice* voice);
void synth_ahx_voice_reset(SynthAHXVoice* voice);

// Voice control
void synth_ahx_voice_note_on(SynthAHXVoice* voice, int note, int velocity);
void synth_ahx_voice_note_off(SynthAHXVoice* voice);
int synth_ahx_voice_is_active(SynthAHXVoice* voice);

// Waveform parameters
void synth_ahx_voice_set_waveform(SynthAHXVoice* voice, SynthAHXWaveform waveform);
void synth_ahx_voice_set_wave_length(SynthAHXVoice* voice, int length); // 4-256 samples

// ADSR envelope (0.0 - 1.0)
void synth_ahx_voice_set_attack(SynthAHXVoice* voice, float attack);
void synth_ahx_voice_set_decay(SynthAHXVoice* voice, float decay);
void synth_ahx_voice_set_sustain(SynthAHXVoice* voice, float sustain);
void synth_ahx_voice_set_release(SynthAHXVoice* voice, float release);

// Filter (0.0 - 1.0)
void synth_ahx_voice_set_filter_type(SynthAHXVoice* voice, SynthAHXFilterType type);
void synth_ahx_voice_set_filter_cutoff(SynthAHXVoice* voice, float cutoff);
void synth_ahx_voice_set_filter_resonance(SynthAHXVoice* voice, float resonance);

// Modulation
void synth_ahx_voice_set_vibrato_depth(SynthAHXVoice* voice, float depth);
void synth_ahx_voice_set_vibrato_speed(SynthAHXVoice* voice, float speed);

// Processing
float synth_ahx_voice_process(SynthAHXVoice* voice, int sample_rate);

#ifdef __cplusplus
}
#endif

#endif // SYNTH_AHX_H
