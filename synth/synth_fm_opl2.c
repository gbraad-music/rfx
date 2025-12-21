/*
 * Regroove OPL2-Style FM Operator Implementation
 */

#include "synth_fm_opl2.h"
#include <stdlib.h>
#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#ifndef M_2PI
#define M_2PI 6.28318530717958647692
#endif

typedef enum {
    ENV_ATTACK = 0,
    ENV_DECAY,
    ENV_SUSTAIN,
    ENV_RELEASE,
    ENV_OFF
} EnvelopeState;

struct SynthFMOperator {
    OPL2Waveform waveform;
    float multiplier;
    float level;

    // Envelope
    float attack;
    float decay;
    float sustain;
    float release;

    EnvelopeState env_state;
    float env_value;

    // Phase accumulator
    float phase;
};

SynthFMOperator* synth_fm_operator_create(void)
{
    SynthFMOperator* op = (SynthFMOperator*)malloc(sizeof(SynthFMOperator));
    if (!op) return NULL;

    op->waveform = OPL2_WAVE_SINE;
    op->multiplier = 1.0f;
    op->level = 1.0f;

    op->attack = 0.01f;
    op->decay = 0.3f;
    op->sustain = 0.7f;
    op->release = 0.1f;

    op->env_state = ENV_OFF;
    op->env_value = 0.0f;
    op->phase = 0.0f;

    return op;
}

void synth_fm_operator_destroy(SynthFMOperator* op)
{
    if (op) free(op);
}

void synth_fm_operator_reset(SynthFMOperator* op)
{
    if (!op) return;
    op->env_state = ENV_OFF;
    op->env_value = 0.0f;
    op->phase = 0.0f;
}

void synth_fm_operator_set_waveform(SynthFMOperator* op, OPL2Waveform wave)
{
    if (op) op->waveform = wave;
}

void synth_fm_operator_set_multiplier(SynthFMOperator* op, float multiplier)
{
    if (!op) return;
    if (multiplier < 0.5f) multiplier = 0.5f;
    if (multiplier > 15.0f) multiplier = 15.0f;
    op->multiplier = multiplier;
}

void synth_fm_operator_set_level(SynthFMOperator* op, float level)
{
    if (!op) return;
    if (level < 0.0f) level = 0.0f;
    if (level > 1.0f) level = 1.0f;
    op->level = level;
}

void synth_fm_operator_set_attack(SynthFMOperator* op, float attack)
{
    if (op) op->attack = attack;
}

void synth_fm_operator_set_decay(SynthFMOperator* op, float decay)
{
    if (op) op->decay = decay;
}

void synth_fm_operator_set_sustain(SynthFMOperator* op, float sustain)
{
    if (op) op->sustain = sustain;
}

void synth_fm_operator_set_release(SynthFMOperator* op, float release)
{
    if (op) op->release = release;
}

void synth_fm_operator_trigger(SynthFMOperator* op)
{
    if (!op) return;
    op->env_state = ENV_ATTACK;
    op->env_value = 0.0f;
}

void synth_fm_operator_release(SynthFMOperator* op)
{
    if (!op) return;
    op->env_state = ENV_RELEASE;
}

bool synth_fm_operator_is_active(SynthFMOperator* op)
{
    return op && op->env_state != ENV_OFF;
}

// Generate waveform sample
static float generate_waveform(OPL2Waveform wave, float phase)
{
    float output = 0.0f;

    switch (wave) {
    case OPL2_WAVE_SINE:
        output = sinf(M_2PI * phase);
        break;

    case OPL2_WAVE_HALF_SINE:
        // Half sine (positive half only)
        if (phase < 0.5f) {
            output = sinf(M_2PI * phase);
        } else {
            output = 0.0f;
        }
        break;

    case OPL2_WAVE_ABS_SINE:
        // Absolute value of sine
        output = fabsf(sinf(M_2PI * phase));
        break;

    case OPL2_WAVE_QUARTER_SINE:
        // Quarter sine (first quarter only)
        if (phase < 0.25f) {
            output = sinf(M_2PI * phase);
        } else {
            output = 0.0f;
        }
        break;

    case OPL2_WAVE_SINE_DOUBLE_HALF:
        // Double frequency half sine
        if (fmodf(phase * 2.0f, 1.0f) < 0.5f) {
            output = sinf(M_2PI * phase * 2.0f);
        } else {
            output = 0.0f;
        }
        break;

    case OPL2_WAVE_ABS_SINE_DOUBLE:
        // Double frequency absolute sine
        output = fabsf(sinf(M_2PI * phase * 2.0f));
        break;

    case OPL2_WAVE_SQUARE:
        output = phase < 0.5f ? 1.0f : -1.0f;
        break;

    case OPL2_WAVE_DERIVED_SQUARE:
        // Derived square (pulse-like)
        output = phase < 0.25f ? 1.0f : 0.0f;
        break;
    }

    return output;
}

float synth_fm_operator_process(SynthFMOperator* op, float base_freq,
                                float modulation, int sample_rate)
{
    if (!op || op->env_state == ENV_OFF) return 0.0f;

    // Process envelope
    float rate = 0.0f;

    switch (op->env_state) {
    case ENV_ATTACK:
        if (op->attack > 0.001f) {
            rate = 1.0f / (op->attack * sample_rate);
            op->env_value += rate;
            if (op->env_value >= 1.0f) {
                op->env_value = 1.0f;
                op->env_state = ENV_DECAY;
            }
        } else {
            op->env_value = 1.0f;
            op->env_state = ENV_DECAY;
        }
        break;

    case ENV_DECAY:
        if (op->decay > 0.001f) {
            rate = (1.0f - op->sustain) / (op->decay * sample_rate);
            op->env_value -= rate;
            if (op->env_value <= op->sustain) {
                op->env_value = op->sustain;
                op->env_state = ENV_SUSTAIN;
            }
        } else {
            op->env_value = op->sustain;
            op->env_state = ENV_SUSTAIN;
        }
        break;

    case ENV_SUSTAIN:
        op->env_value = op->sustain;
        break;

    case ENV_RELEASE:
        if (op->release > 0.001f) {
            rate = op->env_value / (op->release * sample_rate);
            op->env_value -= rate;
            if (op->env_value <= 0.0f) {
                op->env_value = 0.0f;
                op->env_state = ENV_OFF;
            }
        } else {
            op->env_value = 0.0f;
            op->env_state = ENV_OFF;
        }
        break;

    case ENV_OFF:
        return 0.0f;
    }

    // Calculate frequency with modulation
    float freq = base_freq * op->multiplier;

    // Phase modulation (FM synthesis)
    float modulated_phase = op->phase + modulation;

    // Wrap phase
    while (modulated_phase >= 1.0f) modulated_phase -= 1.0f;
    while (modulated_phase < 0.0f) modulated_phase += 1.0f;

    // Generate waveform
    float output = generate_waveform(op->waveform, modulated_phase);

    // Apply envelope and level
    output *= op->env_value * op->level;

    // Advance phase
    float phase_inc = freq / sample_rate;
    op->phase += phase_inc;
    if (op->phase >= 1.0f) {
        op->phase -= 1.0f;
    }

    return output;
}
