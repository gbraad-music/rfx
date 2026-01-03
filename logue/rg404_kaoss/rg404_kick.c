#include "rg404_kick.h"
#include <stdlib.h>

struct RG404Kick {
    float global_tempo_bpm;
    float tempo_multiplier;
    float kick_mix;
    float input_mix;

    float metro_phase;
    float metro_increment;

    int kick_active;
    float envelope_time;  // Time progress in seconds (0 to 0.15)
    float osc_phase;      // Oscillator phase (0 to 1)
};

RG404Kick* rg404_kick_create(void)
{
    RG404Kick* kick = (RG404Kick*)malloc(sizeof(RG404Kick));
    if (!kick) return NULL;

    kick->global_tempo_bpm = 120.0f;
    kick->tempo_multiplier = 1.0f;
    kick->kick_mix = 0.9f;
    kick->input_mix = 0.1f;
    kick->metro_phase = 0.0f;
    kick->metro_increment = 0.0f;
    kick->kick_active = 0;
    kick->envelope_time = 0.0f;
    kick->osc_phase = 0.0f;

    // Calculate initial metro increment
    float effective_bpm = kick->global_tempo_bpm * kick->tempo_multiplier;
    float beats_per_second = effective_bpm / 60.0f;
    kick->metro_increment = beats_per_second / 48000.0f;

    return kick;
}

void rg404_kick_destroy(RG404Kick* kick)
{
    if (kick) {
        free(kick);
    }
}

void rg404_kick_set_tempo_mult(RG404Kick* kick, float mult)
{
    if (!kick) return;
    kick->tempo_multiplier = 0.5f + (mult * 1.5f);

    float effective_bpm = kick->global_tempo_bpm * kick->tempo_multiplier;
    if (effective_bpm < 30.0f) effective_bpm = 30.0f;
    if (effective_bpm > 300.0f) effective_bpm = 300.0f;
    float beats_per_second = effective_bpm / 60.0f;
    kick->metro_increment = beats_per_second / 48000.0f;
}

void rg404_kick_set_mix(RG404Kick* kick, float mix)
{
    if (!kick) return;
    kick->kick_mix = mix;
}

void rg404_kick_set_tempo(RG404Kick* kick, float bpm)
{
    if (!kick) return;
    kick->global_tempo_bpm = bpm;

    float effective_bpm = kick->global_tempo_bpm * kick->tempo_multiplier;
    if (effective_bpm < 30.0f) effective_bpm = 30.0f;
    if (effective_bpm > 300.0f) effective_bpm = 300.0f;
    float beats_per_second = effective_bpm / 60.0f;
    kick->metro_increment = beats_per_second / 48000.0f;
}

void rg404_kick_process(RG404Kick* kick, const float* in_l, const float* in_r,
                        float* out_l, float* out_r, int sample_rate)
{
    if (!kick) return;

    // Metro
    kick->metro_phase += kick->metro_increment;
    if (kick->metro_phase >= 1.0f) {
        kick->metro_phase -= 1.0f;
        kick->kick_active = 1;
        kick->envelope_time = 0.0f;
        kick->osc_phase = 0.0f;
    }

    // Generate kick - 909-style with pitch envelope
    float kick_out = 0.0f;
    if (kick->kick_active) {
        const float kick_duration = 0.4f;  // 400ms total

        if (kick->envelope_time < kick_duration) {
            // Amplitude envelope - fast attack, exponential decay
            float amp_env = 1.0f - (kick->envelope_time / kick_duration);
            amp_env = amp_env * amp_env * amp_env;  // Cubic decay

            // Pitch envelope - sweep from 180Hz down to 40Hz
            float pitch_env_time = kick->envelope_time / 0.03f;  // 30ms pitch envelope
            if (pitch_env_time > 1.0f) pitch_env_time = 1.0f;
            pitch_env_time = pitch_env_time * pitch_env_time;  // Exponential curve

            float freq = 180.0f - (pitch_env_time * 140.0f);  // 180Hz -> 40Hz

            // Phase increment for oscillator
            float phase_inc = freq / (float)sample_rate;
            kick->osc_phase += phase_inc;
            if (kick->osc_phase >= 1.0f) kick->osc_phase -= 1.0f;

            // Simple sine approximation using parabola
            float t = kick->osc_phase * 2.0f - 1.0f;  // -1 to 1
            float sine;
            if (t >= 0.0f) {
                sine = 1.0f - 4.0f * (t - 0.5f) * (t - 0.5f);
            } else {
                sine = -(1.0f - 4.0f * (t + 0.5f) * (t + 0.5f));
            }

            // Apply amplitude envelope - boosted for louder bass
            kick_out = sine * amp_env * 1.5f;

            kick->envelope_time += 1.0f / (float)sample_rate;
        } else {
            kick->kick_active = 0;
        }
    }

    // Mix
    *out_l = (*in_l) * kick->input_mix + kick_out * kick->kick_mix;
    *out_r = (*in_r) * kick->input_mix + kick_out * kick->kick_mix;
}
