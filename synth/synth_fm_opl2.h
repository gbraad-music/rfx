/*
 * Regroove OPL2-Style FM Operator
 * 2-operator FM synthesis (Yamaha YM3812 compatible)
 */

#ifndef SYNTH_FM_OPL2_H
#define SYNTH_FM_OPL2_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

// OPL2 waveforms
typedef enum {
    OPL2_WAVE_SINE = 0,
    OPL2_WAVE_HALF_SINE,
    OPL2_WAVE_ABS_SINE,
    OPL2_WAVE_QUARTER_SINE,
    OPL2_WAVE_SINE_DOUBLE_HALF,
    OPL2_WAVE_ABS_SINE_DOUBLE,
    OPL2_WAVE_SQUARE,
    OPL2_WAVE_DERIVED_SQUARE
} OPL2Waveform;

typedef struct SynthFMOperator SynthFMOperator;

/**
 * Create an FM operator
 */
SynthFMOperator* synth_fm_operator_create(void);

/**
 * Destroy an FM operator
 */
void synth_fm_operator_destroy(SynthFMOperator* op);

/**
 * Reset operator state
 */
void synth_fm_operator_reset(SynthFMOperator* op);

/**
 * Set operator waveform
 */
void synth_fm_operator_set_waveform(SynthFMOperator* op, OPL2Waveform wave);

/**
 * Set frequency multiplier (0.5x to 15x)
 */
void synth_fm_operator_set_multiplier(SynthFMOperator* op, float multiplier);

/**
 * Set output level (0.0 to 1.0)
 */
void synth_fm_operator_set_level(SynthFMOperator* op, float level);

/**
 * Set ADSR envelope
 */
void synth_fm_operator_set_attack(SynthFMOperator* op, float attack);
void synth_fm_operator_set_decay(SynthFMOperator* op, float decay);
void synth_fm_operator_set_sustain(SynthFMOperator* op, float sustain);
void synth_fm_operator_set_release(SynthFMOperator* op, float release);

/**
 * Trigger operator (key on)
 */
void synth_fm_operator_trigger(SynthFMOperator* op);

/**
 * Release operator (key off)
 */
void synth_fm_operator_release(SynthFMOperator* op);

/**
 * Process operator with optional FM modulation input
 * @param base_freq Base frequency in Hz
 * @param modulation FM modulation input (0.0 for carrier)
 * @param sample_rate Sample rate
 * @return Operator output
 */
float synth_fm_operator_process(SynthFMOperator* op, float base_freq,
                                float modulation, int sample_rate);

/**
 * Check if operator is active
 */
bool synth_fm_operator_is_active(SynthFMOperator* op);

#ifdef __cplusplus
}
#endif

#endif // SYNTH_FM_OPL2_H
