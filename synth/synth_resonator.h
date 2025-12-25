/*
 * Resonator Module - TR-909 style resonant filter
 * Based on biquad filter with exponential decay
 */

#ifndef SYNTH_RESONATOR_H
#define SYNTH_RESONATOR_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    float a1, a2;     // feedback coefficients
    float b0;         // input gain
    float z1, z2;     // state
} SynthResonator;

SynthResonator* synth_resonator_create(void);
void synth_resonator_destroy(SynthResonator* r);
void synth_resonator_reset(SynthResonator* r);

// f0 = center frequency (Hz)
// decay = time in seconds to -60 dB
void synth_resonator_set_params(SynthResonator* r, float f0, float decay, float sampleRate);

// excite the resonator (909 uses a short pulse)
void synth_resonator_strike(SynthResonator* r, float strength);

// process one sample
float synth_resonator_process(SynthResonator* r, float input);

#ifdef __cplusplus
}
#endif

#endif // SYNTH_RESONATOR_H
