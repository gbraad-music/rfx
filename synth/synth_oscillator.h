/*
 * Regroove Synthesizer Oscillator
 * Band-limited oscillator with sawtooth and square waveforms
 */

#ifndef SYNTH_OSCILLATOR_H
#define SYNTH_OSCILLATOR_H

#include "synth_common.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    SYNTH_OSC_SAW,
    SYNTH_OSC_SQUARE,
    SYNTH_OSC_TRIANGLE,
    SYNTH_OSC_SINE
} SynthOscWaveform;

typedef struct SynthOscillator SynthOscillator;

// Lifecycle
SynthOscillator* synth_oscillator_create(void);
void synth_oscillator_destroy(SynthOscillator* osc);
void synth_oscillator_reset(SynthOscillator* osc);

// Configuration
void synth_oscillator_set_waveform(SynthOscillator* osc, SynthOscWaveform waveform);
void synth_oscillator_set_frequency(SynthOscillator* osc, float freq);
void synth_oscillator_set_pulse_width(SynthOscillator* osc, float width); // For square wave (0.0-1.0)

// Processing
float synth_oscillator_process(SynthOscillator* osc, int sample_rate);

// Phase manipulation
void synth_oscillator_set_phase(SynthOscillator* osc, float phase); // 0.0-1.0

#ifdef __cplusplus
}
#endif

#endif // SYNTH_OSCILLATOR_H
