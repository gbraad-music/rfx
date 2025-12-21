/*
 * Regroove BBD-Style Chorus Effect
 * Based on Juno-106 chorus circuit
 */

#ifndef SYNTH_CHORUS_H
#define SYNTH_CHORUS_H

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    CHORUS_OFF = 0,
    CHORUS_I,      // Chorus I (single delay tap)
    CHORUS_II      // Chorus II (dual delay tap)
} ChorusMode;

typedef struct SynthChorus SynthChorus;

/**
 * Create a chorus effect
 */
SynthChorus* synth_chorus_create(void);

/**
 * Destroy a chorus effect
 */
void synth_chorus_destroy(SynthChorus* chorus);

/**
 * Reset chorus state
 */
void synth_chorus_reset(SynthChorus* chorus);

/**
 * Set chorus mode (off, I, II)
 */
void synth_chorus_set_mode(SynthChorus* chorus, ChorusMode mode);

/**
 * Set chorus rate (LFO frequency in Hz, typically 0.1-10Hz)
 */
void synth_chorus_set_rate(SynthChorus* chorus, float rate_hz);

/**
 * Set chorus depth (0.0 to 1.0)
 */
void synth_chorus_set_depth(SynthChorus* chorus, float depth);

/**
 * Process stereo chorus
 * @param input Mono input sample
 * @param out_left Output left channel
 * @param out_right Output right channel
 * @param sample_rate Sample rate
 */
void synth_chorus_process(SynthChorus* chorus, float input,
                          float* out_left, float* out_right, int sample_rate);

#ifdef __cplusplus
}
#endif

#endif // SYNTH_CHORUS_H
