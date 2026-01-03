#include "rg404_kick.h"
#include <stdlib.h>

struct RG404Kick {
    float global_tempo_bpm;
    float rhythm_variation;  // 0.0 = pure 4-on-floor, 1.0 = max syncopation
    float kick_mix;
    float input_mix;
    float drive_amount;      // 1.0 = clean, 3.0 = max overdrive

    float metro_phase;
    float metro_increment;
    int step_count;       // 16th note counter (0-15, one bar)

    int kick_active;
    float envelope_time;  // Time progress in seconds (0 to 0.15)
    float osc_phase;      // Oscillator phase (0 to 1)
};

RG404Kick* rg404_kick_create(void)
{
    RG404Kick* kick = (RG404Kick*)malloc(sizeof(RG404Kick));
    if (!kick) return NULL;

    kick->global_tempo_bpm = 120.0f;
    kick->rhythm_variation = 0.0f;  // Pure 4-on-floor by default
    kick->kick_mix = 0.9f;
    kick->input_mix = 0.1f;
    kick->drive_amount = 1.0f;      // Clean by default
    kick->metro_phase = 0.0f;
    kick->metro_increment = 0.0f;
    kick->step_count = 0;
    kick->kick_active = 0;
    kick->envelope_time = 0.0f;
    kick->osc_phase = 0.0f;

    // Calculate initial metro increment (16th notes = 4x beat rate)
    float beats_per_second = kick->global_tempo_bpm / 60.0f;
    float sixteenths_per_second = beats_per_second * 4.0f;
    kick->metro_increment = sixteenths_per_second / 48000.0f;

    return kick;
}

void rg404_kick_destroy(RG404Kick* kick)
{
    if (kick) {
        free(kick);
    }
}

void rg404_kick_set_rhythm(RG404Kick* kick, float rhythm)
{
    if (!kick) return;
    kick->rhythm_variation = rhythm;
}

void rg404_kick_set_mix(RG404Kick* kick, float mix)
{
    if (!kick) return;
    kick->kick_mix = mix;
}

void rg404_kick_set_drive(RG404Kick* kick, float drive)
{
    if (!kick) return;
    kick->drive_amount = drive;
}

void rg404_kick_set_tempo(RG404Kick* kick, float bpm)
{
    if (!kick) return;
    kick->global_tempo_bpm = bpm;

    if (bpm < 30.0f) bpm = 30.0f;
    if (bpm > 300.0f) bpm = 300.0f;
    float beats_per_second = bpm / 60.0f;
    float sixteenths_per_second = beats_per_second * 4.0f;
    kick->metro_increment = sixteenths_per_second / 48000.0f;
}

void rg404_kick_process(RG404Kick* kick, const float* in_l, const float* in_r,
                        float* out_l, float* out_r, int sample_rate)
{
    if (!kick) return;

    // Metro - runs at 16th note resolution
    kick->metro_phase += kick->metro_increment;
    if (kick->metro_phase >= 1.0f) {
        kick->metro_phase -= 1.0f;

        // Determine if we should trigger a kick on this 16th note
        int should_trigger = 0;

        // Main beats are steps 0, 4, 8, 12 (quarter notes)
        // Pure 4-on-the-floor: kick on every quarter note
        if (kick->step_count == 0 || kick->step_count == 4 ||
            kick->step_count == 8 || kick->step_count == 12) {
            should_trigger = 1;
        }

        // Add syncopation based on rhythm_variation
        if (kick->rhythm_variation > 0.2f) {
            // Add kick before beat 4: step 14 (last 16th of bar)
            if (kick->step_count == 14) {
                should_trigger = 1;
            }
        }

        if (kick->rhythm_variation > 0.5f) {
            // Add more syncopation: step 6 (16th after beat 2)
            if (kick->step_count == 6) {
                should_trigger = 1;
            }
        }

        if (kick->rhythm_variation > 0.8f) {
            // Dense pattern: add step 10 (16th after beat 3)
            if (kick->step_count == 10) {
                should_trigger = 1;
            }
        }

        if (should_trigger) {
            kick->kick_active = 1;
            kick->envelope_time = 0.0f;
            kick->osc_phase = 0.0f;
        }

        // Advance step counter (16 steps = 1 bar)
        kick->step_count++;
        if (kick->step_count >= 16) {
            kick->step_count = 0;
        }
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

            // Apply drive/saturation
            if (kick->drive_amount > 1.0f) {
                kick_out *= kick->drive_amount;
                // Soft clipping using tanh approximation
                // tanh(x) ≈ x / (1 + |x|) for a simpler soft clip
                if (kick_out > 0.0f) {
                    kick_out = kick_out / (1.0f + kick_out);
                } else {
                    kick_out = kick_out / (1.0f - kick_out);
                }
                kick_out *= 1.5f; // Compensate for clipping loss
            }

            kick->envelope_time += 1.0f / (float)sample_rate;
        } else {
            kick->kick_active = 0;
        }
    }

    // Mix
    *out_l = (*in_l) * kick->input_mix + kick_out * kick->kick_mix;
    *out_r = (*in_r) * kick->input_mix + kick_out * kick->kick_mix;
}
