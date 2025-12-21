/*
 * TB-303 Style Envelope Generator
 * Exponential attack/decay with proper 303 characteristics
 */

#ifndef SYNTH_ENVELOPE_TB303_H
#define SYNTH_ENVELOPE_TB303_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct SynthEnvelopeTB303 SynthEnvelopeTB303;

// Lifecycle
SynthEnvelopeTB303* synth_envelope_tb303_create(void);
void synth_envelope_tb303_destroy(SynthEnvelopeTB303* env);
void synth_envelope_tb303_reset(SynthEnvelopeTB303* env);

// Control
void synth_envelope_tb303_trigger(SynthEnvelopeTB303* env);
void synth_envelope_tb303_release(SynthEnvelopeTB303* env);

// Parameters
void synth_envelope_tb303_set_attack(SynthEnvelopeTB303* env, float attack);
void synth_envelope_tb303_set_decay(SynthEnvelopeTB303* env, float decay);

// Processing
float synth_envelope_tb303_process(SynthEnvelopeTB303* env, int sample_rate);
int synth_envelope_tb303_is_active(SynthEnvelopeTB303* env);

#ifdef __cplusplus
}
#endif

#endif // SYNTH_ENVELOPE_TB303_H
