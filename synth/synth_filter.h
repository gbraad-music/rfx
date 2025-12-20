/*
 * Regroove Synthesizer Filter
 * TB303-style resonant low-pass filter
 */

#ifndef SYNTH_FILTER_H
#define SYNTH_FILTER_H

#include "synth_common.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    SYNTH_FILTER_LPF,
    SYNTH_FILTER_HPF,
    SYNTH_FILTER_BPF
} SynthFilterType;

typedef struct SynthFilter SynthFilter;

// Lifecycle
SynthFilter* synth_filter_create(void);
void synth_filter_destroy(SynthFilter* filter);
void synth_filter_reset(SynthFilter* filter);

// Configuration
void synth_filter_set_type(SynthFilter* filter, SynthFilterType type);
void synth_filter_set_cutoff(SynthFilter* filter, float cutoff);     // 0.0-1.0 (normalized frequency)
void synth_filter_set_resonance(SynthFilter* filter, float resonance); // 0.0-1.0

// Processing
float synth_filter_process(SynthFilter* filter, float input, int sample_rate);

#ifdef __cplusplus
}
#endif

#endif // SYNTH_FILTER_H
