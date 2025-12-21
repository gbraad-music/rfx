/*
 * Regroove MS-20 Style Dual Filter
 * HPF + LPF cascade with aggressive resonance
 * Based on Korg MS-20 filter topology
 */

#ifndef SYNTH_FILTER_MS20_H
#define SYNTH_FILTER_MS20_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct SynthFilterMS20 SynthFilterMS20;

/**
 * Create a new MS-20 dual filter
 */
SynthFilterMS20* synth_filter_ms20_create(void);

/**
 * Destroy an MS-20 filter
 */
void synth_filter_ms20_destroy(SynthFilterMS20* filter);

/**
 * Reset filter state to zero
 */
void synth_filter_ms20_reset(SynthFilterMS20* filter);

/**
 * Set highpass filter cutoff (0.0 to 1.0)
 */
void synth_filter_ms20_set_hpf_cutoff(SynthFilterMS20* filter, float cutoff);

/**
 * Set highpass filter peak/resonance (0.0 to 1.0)
 */
void synth_filter_ms20_set_hpf_peak(SynthFilterMS20* filter, float peak);

/**
 * Set lowpass filter cutoff (0.0 to 1.0)
 */
void synth_filter_ms20_set_lpf_cutoff(SynthFilterMS20* filter, float cutoff);

/**
 * Set lowpass filter peak/resonance (0.0 to 1.0)
 */
void synth_filter_ms20_set_lpf_peak(SynthFilterMS20* filter, float peak);

/**
 * Process a single sample through the MS-20 dual filter
 * Signal path: input → HPF → LPF → output
 */
float synth_filter_ms20_process(SynthFilterMS20* filter, float input, int sample_rate);

#ifdef __cplusplus
}
#endif

#endif // SYNTH_FILTER_MS20_H
