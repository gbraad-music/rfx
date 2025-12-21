/*
 * Regroove Low Frequency Oscillator (LFO)
 */

#ifndef SYNTH_LFO_H
#define SYNTH_LFO_H

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    SYNTH_LFO_SINE = 0,
    SYNTH_LFO_TRIANGLE,
    SYNTH_LFO_SQUARE,
    SYNTH_LFO_SAW_UP,
    SYNTH_LFO_SAW_DOWN,
    SYNTH_LFO_RANDOM
} SynthLFOWaveform;

typedef struct SynthLFO SynthLFO;

/**
 * Create a new LFO
 */
SynthLFO* synth_lfo_create(void);

/**
 * Destroy an LFO
 */
void synth_lfo_destroy(SynthLFO* lfo);

/**
 * Reset LFO phase to zero
 */
void synth_lfo_reset(SynthLFO* lfo);

/**
 * Set LFO waveform
 */
void synth_lfo_set_waveform(SynthLFO* lfo, SynthLFOWaveform waveform);

/**
 * Set LFO frequency in Hz (typically 0.1 to 20 Hz)
 */
void synth_lfo_set_frequency(SynthLFO* lfo, float freq_hz);

/**
 * Process one sample, returns value in range -1.0 to +1.0
 */
float synth_lfo_process(SynthLFO* lfo, int sample_rate);

#ifdef __cplusplus
}
#endif

#endif // SYNTH_LFO_H
