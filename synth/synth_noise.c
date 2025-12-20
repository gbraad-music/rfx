#include "synth_noise.h"
#include <stdlib.h>
#include <stdint.h>

struct SynthNoise {
    uint32_t seed;
};

SynthNoise* synth_noise_create(void)
{
    SynthNoise* noise = (SynthNoise*)malloc(sizeof(SynthNoise));
    if (!noise) return NULL;

    // Initialize with a random-ish seed based on address
    noise->seed = (uint32_t)(uintptr_t)noise;
    if (noise->seed == 0) noise->seed = 1;

    return noise;
}

void synth_noise_destroy(SynthNoise* noise)
{
    if (noise) {
        free(noise);
    }
}

float synth_noise_process(SynthNoise* noise)
{
    if (!noise) return 0.0f;

    // Linear congruential generator (LCG) for white noise
    // Fast and good enough for percussion sounds
    noise->seed = (noise->seed * 1664525 + 1013904223);

    // Convert to float in range [-1.0, 1.0]
    int32_t signed_val = (int32_t)noise->seed;
    return (float)signed_val / 2147483648.0f;
}

void synth_noise_reset(SynthNoise* noise)
{
    if (!noise) return;

    // Reset to initial seed (don't re-randomize, for consistency)
    noise->seed = (uint32_t)(uintptr_t)noise;
    if (noise->seed == 0) noise->seed = 1;
}

