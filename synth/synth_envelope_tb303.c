/*
 * TB-303 Style Envelope Generator
 * Exponential attack/decay with proper 303 characteristics
 */

#include "synth_envelope_tb303.h"
#include <stdlib.h>
#include <math.h>

struct SynthEnvelopeTB303 {
    float attack;
    float decay;
    float env;
    int state;  // 0=idle, 1=attack, 2=decay
};

SynthEnvelopeTB303* synth_envelope_tb303_create(void)
{
    SynthEnvelopeTB303* e = calloc(1, sizeof(SynthEnvelopeTB303));
    if (!e) return NULL;

    e->attack = 0.003f;  // ~3ms
    e->decay  = 0.2f;    // ~200ms
    e->env = 0.0f;
    e->state = 0;

    return e;
}

void synth_envelope_tb303_destroy(SynthEnvelopeTB303* env)
{
    if (env) free(env);
}

void synth_envelope_tb303_reset(SynthEnvelopeTB303* env)
{
    if (!env) return;

    env->env = 0.0f;
    env->state = 0;
}

void synth_envelope_tb303_trigger(SynthEnvelopeTB303* env)
{
    if (!env) return;

    env->env = 0.0f;
    env->state = 1;  // Attack
}

void synth_envelope_tb303_release(SynthEnvelopeTB303* env)
{
    if (!env) return;

    env->state = 2;  // Decay
}

void synth_envelope_tb303_set_attack(SynthEnvelopeTB303* env, float attack)
{
    if (!env) return;
    if (attack < 0.001f) attack = 0.001f;
    if (attack > 5.0f) attack = 5.0f;
    env->attack = attack;
}

void synth_envelope_tb303_set_decay(SynthEnvelopeTB303* env, float decay)
{
    if (!env) return;
    if (decay < 0.01f) decay = 0.01f;
    if (decay > 5.0f) decay = 5.0f;
    env->decay = decay;
}

float synth_envelope_tb303_process(SynthEnvelopeTB303* env, int sample_rate)
{
    if (!env) return 0.0f;

    if (env->state == 1) {
        // Exponential attack
        float a = expf(-1.0f / (env->attack * (float)sample_rate));
        env->env = 1.0f + (env->env - 1.0f) * a;
        if (env->env > 0.99f) {
            env->env = 1.0f;
            env->state = 2;  // Move to decay
        }
    }
    else if (env->state == 2) {
        // Exponential decay
        float d = expf(-1.0f / (env->decay * (float)sample_rate));
        env->env *= d;
        if (env->env < 0.0001f) {
            env->env = 0.0f;
            env->state = 0;  // Idle
        }
    }

    return env->env;
}

int synth_envelope_tb303_is_active(SynthEnvelopeTB303* env)
{
    if (!env) return 0;
    return env->state != 0;
}
