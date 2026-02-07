/*
 * Regroove Synthesizer Envelope Generator Implementation
 */

#include "synth_envelope.h"
#include <stdlib.h>

struct SynthEnvelope {
    // Parameters (in seconds)
    float attack;
    float decay;
    float sustain;   // Level (0.0-1.0)
    float release;

    // State
    SynthEnvStage stage;
    float level;
    float phase;         // Phase within current stage (0.0-1.0)
    float release_start; // Level at start of release phase
};

SynthEnvelope* synth_envelope_create(void)
{
    SynthEnvelope* env = (SynthEnvelope*)malloc(sizeof(SynthEnvelope));
    if (!env) return NULL;

    env->attack = 0.01f;
    env->decay = 0.1f;
    env->sustain = 0.7f;
    env->release = 0.2f;

    synth_envelope_reset(env);
    return env;
}

void synth_envelope_destroy(SynthEnvelope* env)
{
    if (env) free(env);
}

void synth_envelope_reset(SynthEnvelope* env)
{
    if (!env) return;
    env->stage = SYNTH_ENV_IDLE;
    env->level = 0.0f;
    env->phase = 0.0f;
}

void synth_envelope_set_attack(SynthEnvelope* env, float attack)
{
    if (env) env->attack = attack < 0.001f ? 0.001f : attack;
}

void synth_envelope_set_decay(SynthEnvelope* env, float decay)
{
    if (env) env->decay = decay < 0.001f ? 0.001f : decay;
}

void synth_envelope_set_sustain(SynthEnvelope* env, float sustain)
{
    if (!env) return;
    env->sustain = sustain < 0.0f ? 0.0f : (sustain > 1.0f ? 1.0f : sustain);
}

void synth_envelope_set_release(SynthEnvelope* env, float release)
{
    if (env) env->release = release < 0.001f ? 0.001f : release;
}

void synth_envelope_trigger(SynthEnvelope* env)
{
    if (!env) return;
    env->stage = SYNTH_ENV_ATTACK;
    env->phase = 0.0f;
    // Don't reset level - allows for re-triggering
}

void synth_envelope_release(SynthEnvelope* env)
{
    if (!env) return;
    if (env->stage != SYNTH_ENV_IDLE && env->stage != SYNTH_ENV_RELEASE) {
        env->release_start = env->level;  // Capture level at start of release
        env->stage = SYNTH_ENV_RELEASE;
        env->phase = 0.0f;
    }
}

int synth_envelope_is_active(SynthEnvelope* env)
{
    return env && env->stage != SYNTH_ENV_IDLE;
}

SynthEnvStage synth_envelope_get_stage(SynthEnvelope* env)
{
    return env ? env->stage : SYNTH_ENV_IDLE;
}

float synth_envelope_process(SynthEnvelope* env, int sample_rate)
{
    if (!env || env->stage == SYNTH_ENV_IDLE) {
        return 0.0f;
    }

    float dt = 1.0f / (float)sample_rate;

    switch (env->stage) {
    case SYNTH_ENV_ATTACK:
        if (env->attack > 0.0f) {
            env->phase += dt / env->attack;
            if (env->phase >= 1.0f) {
                env->phase = 0.0f;
                env->level = 1.0f;
                env->stage = SYNTH_ENV_DECAY;
            } else {
                env->level = env->phase;
            }
        } else {
            env->level = 1.0f;
            env->stage = SYNTH_ENV_DECAY;
        }
        break;

    case SYNTH_ENV_DECAY:
        if (env->decay > 0.0f) {
            env->phase += dt / env->decay;
            if (env->phase >= 1.0f) {
                env->phase = 0.0f;
                env->level = env->sustain;
                env->stage = SYNTH_ENV_SUSTAIN;
            } else {
                env->level = 1.0f - env->phase * (1.0f - env->sustain);
            }
        } else {
            env->level = env->sustain;
            env->stage = SYNTH_ENV_SUSTAIN;
        }
        break;

    case SYNTH_ENV_SUSTAIN:
        env->level = env->sustain;
        break;

    case SYNTH_ENV_RELEASE:
        if (env->release > 0.0f) {
            env->phase += dt / env->release;
            if (env->phase >= 1.0f) {
                env->level = 0.0f;
                env->stage = SYNTH_ENV_IDLE;
            } else {
                env->level = env->release_start * (1.0f - env->phase);
            }
        } else {
            env->level = 0.0f;
            env->stage = SYNTH_ENV_IDLE;
        }
        break;

    case SYNTH_ENV_IDLE:
        env->level = 0.0f;
        break;
    }

    return env->level;
}
