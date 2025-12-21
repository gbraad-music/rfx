/*
 * Regroove Moog Ladder Filter
 * 4-pole (24dB/octave) resonant lowpass filter
 * Based on simplified Huovilainen model
 */

#ifndef SYNTH_FILTER_LADDER_H
#define SYNTH_FILTER_LADDER_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct SynthFilterLadder SynthFilterLadder;

/**
 * Create a new ladder filter
 */
SynthFilterLadder* synth_filter_ladder_create(void);

/**
 * Destroy a ladder filter
 */
void synth_filter_ladder_destroy(SynthFilterLadder* filter);

/**
 * Reset filter state to zero
 */
void synth_filter_ladder_reset(SynthFilterLadder* filter);

/**
 * Set filter cutoff frequency (0.0 to 1.0)
 * 0.0 = ~20Hz, 1.0 = ~20kHz (sample rate dependent)
 */
void synth_filter_ladder_set_cutoff(SynthFilterLadder* filter, float cutoff);

/**
 * Set filter resonance (0.0 to 1.0)
 * 0.0 = no resonance, 1.0 = self-oscillation
 */
void synth_filter_ladder_set_resonance(SynthFilterLadder* filter, float resonance);

/**
 * Process a single sample through the ladder filter
 */
float synth_filter_ladder_process(SynthFilterLadder* filter, float input, int sample_rate);

#ifdef __cplusplus
}
#endif

#endif // SYNTH_FILTER_LADDER_H
