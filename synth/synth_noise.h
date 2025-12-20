#ifndef SYNTH_NOISE_H
#define SYNTH_NOISE_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct SynthNoise SynthNoise;

// Create/destroy noise generator
SynthNoise* synth_noise_create(void);
void synth_noise_destroy(SynthNoise* noise);

// Generate noise sample
float synth_noise_process(SynthNoise* noise);

// Reset internal state
void synth_noise_reset(SynthNoise* noise);

#ifdef __cplusplus
}
#endif

#endif // SYNTH_NOISE_H
