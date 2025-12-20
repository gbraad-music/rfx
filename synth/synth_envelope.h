/*
 * Regroove Synthesizer Envelope Generator
 * ADSR envelope for amplitude and filter modulation
 */

#ifndef SYNTH_ENVELOPE_H
#define SYNTH_ENVELOPE_H

#include "synth_common.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    SYNTH_ENV_IDLE,
    SYNTH_ENV_ATTACK,
    SYNTH_ENV_DECAY,
    SYNTH_ENV_SUSTAIN,
    SYNTH_ENV_RELEASE
} SynthEnvStage;

typedef struct SynthEnvelope SynthEnvelope;

// Lifecycle
SynthEnvelope* synth_envelope_create(void);
void synth_envelope_destroy(SynthEnvelope* env);
void synth_envelope_reset(SynthEnvelope* env);

// Configuration (times in seconds)
void synth_envelope_set_attack(SynthEnvelope* env, float attack);
void synth_envelope_set_decay(SynthEnvelope* env, float decay);
void synth_envelope_set_sustain(SynthEnvelope* env, float sustain);   // 0.0-1.0 level
void synth_envelope_set_release(SynthEnvelope* env, float release);

// Control
void synth_envelope_trigger(SynthEnvelope* env);
void synth_envelope_release(SynthEnvelope* env);

// Processing
float synth_envelope_process(SynthEnvelope* env, int sample_rate);

// Status
int synth_envelope_is_active(SynthEnvelope* env);
SynthEnvStage synth_envelope_get_stage(SynthEnvelope* env);

#ifdef __cplusplus
}
#endif

#endif // SYNTH_ENVELOPE_H
