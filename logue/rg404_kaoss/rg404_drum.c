#include "rg404_drum.h"
#include <stdlib.h>

struct RG404Drum {
    float global_tempo_bpm;
    float kick_density;      // 0.0 = no kicks, 1.0 = max kick variation
    float snare_variation;   // 0.0 = no snare, 1.0 = max snare variation
    float kick_mix;
    float input_mix;
    float drive_amount;      // 1.0 = clean, 3.0 = max overdrive

    float metro_phase;
    float metro_increment;
    int step_count;       // 16th note counter (0-15, one bar)

    // Kick state
    int kick_active;
    float kick_envelope_time;
    float osc_phase;      // Oscillator phase (0 to 1)

    // Snare state
    int snare_active;
    float snare_envelope_time;
    unsigned int noise_seed;  // For simple noise generation
};

RG404Drum* rg404_drum_create(void)
{
    RG404Drum* drum = (RG404Drum*)malloc(sizeof(RG404Drum));
    if (!drum) return NULL;

    drum->global_tempo_bpm = 120.0f;
    drum->kick_density = 0.5f;      // Some kicks by default
    drum->snare_variation = 0.0f;   // No snare by default
    drum->kick_mix = 0.9f;
    drum->input_mix = 0.1f;
    drum->drive_amount = 1.0f;      // Clean by default
    drum->metro_phase = 0.0f;
    drum->metro_increment = 0.0f;
    drum->step_count = 0;

    // Kick state
    drum->kick_active = 0;
    drum->kick_envelope_time = 0.0f;
    drum->osc_phase = 0.0f;

    // Snare state
    drum->snare_active = 0;
    drum->snare_envelope_time = 0.0f;
    drum->noise_seed = 12345;  // Initial seed for noise generator

    // Calculate initial metro increment (16th notes = 4x beat rate)
    float beats_per_second = drum->global_tempo_bpm / 60.0f;
    float sixteenths_per_second = beats_per_second * 4.0f;
    drum->metro_increment = sixteenths_per_second / 48000.0f;

    return drum;
}

void rg404_drum_destroy(RG404Drum* drum)
{
    if (drum) {
        free(drum);
    }
}

void rg404_drum_set_kick_density(RG404Drum* drum, float density)
{
    if (!drum) return;
    drum->kick_density = density;
}

void rg404_drum_set_snare_variation(RG404Drum* drum, float variation)
{
    if (!drum) return;
    drum->snare_variation = variation;
}

void rg404_drum_set_mix(RG404Drum* drum, float mix)
{
    if (!drum) return;
    drum->kick_mix = mix;
}

void rg404_drum_set_drive(RG404Drum* drum, float drive)
{
    if (!drum) return;
    drum->drive_amount = drive;
}

void rg404_drum_set_tempo(RG404Drum* drum, float bpm)
{
    if (!drum) return;
    drum->global_tempo_bpm = bpm;

    if (bpm < 30.0f) bpm = 30.0f;
    if (bpm > 300.0f) bpm = 300.0f;
    float beats_per_second = bpm / 60.0f;
    float sixteenths_per_second = beats_per_second * 4.0f;
    drum->metro_increment = sixteenths_per_second / 48000.0f;
}

void rg404_drum_process(RG404Drum* drum, const float* in_l, const float* in_r,
                        float* out_l, float* out_r, int sample_rate)
{
    if (!drum) return;

    // Metro - runs at 16th note resolution
    drum->metro_phase += drum->metro_increment;
    if (drum->metro_phase >= 1.0f) {
        drum->metro_phase -= 1.0f;

        // Kick triggers - based on kick_density
        // kick_density = 0.0: no kicks
        // kick_density > 0.0: start with main beats
        // kick_density > 0.2: add syncopation
        int should_trigger_kick = 0;

        if (drum->kick_density > 0.01f) {
            // Main beats: steps 0, 4, 8, 12 (quarter notes = 4-on-floor)
            if (drum->step_count == 0 || drum->step_count == 4 ||
                drum->step_count == 8 || drum->step_count == 12) {
                should_trigger_kick = 1;
            }

            // Add syncopation based on kick_density
            if (drum->kick_density > 0.2f) {
                // Add kick before beat 4: step 14 (last 16th of bar)
                if (drum->step_count == 14) {
                    should_trigger_kick = 1;
                }
            }

            if (drum->kick_density > 0.5f) {
                // Add more syncopation: step 6 (16th after beat 2)
                if (drum->step_count == 6) {
                    should_trigger_kick = 1;
                }
            }

            if (drum->kick_density > 0.8f) {
                // Dense pattern: add step 10 (16th after beat 3)
                if (drum->step_count == 10) {
                    should_trigger_kick = 1;
                }
            }
        }

        if (should_trigger_kick) {
            drum->kick_active = 1;
            drum->kick_envelope_time = 0.0f;
            drum->osc_phase = 0.0f;
        }

        // Snare triggers - based on snare_variation
        // snare_variation = 0.0: no snare
        // snare_variation > 0.0: backbeats (steps 4, 12 = beats 2, 4)
        // snare_variation > 0.3: add more hits
        int should_trigger_snare = 0;

        if (drum->snare_variation > 0.01f) {
            // Backbeats: steps 4, 12 (beats 2 and 4 - classic snare pattern)
            if (drum->step_count == 4 || drum->step_count == 12) {
                should_trigger_snare = 1;
            }

            // Add variation based on snare_variation
            if (drum->snare_variation > 0.3f) {
                // Add ghost note: step 2 (16th after beat 1)
                if (drum->step_count == 2) {
                    should_trigger_snare = 1;
                }
            }

            if (drum->snare_variation > 0.6f) {
                // Add more ghost notes: step 10 (16th after beat 3)
                if (drum->step_count == 10) {
                    should_trigger_snare = 1;
                }
            }

            if (drum->snare_variation > 0.85f) {
                // Dense pattern: add steps 6, 14
                if (drum->step_count == 6 || drum->step_count == 14) {
                    should_trigger_snare = 1;
                }
            }
        }

        if (should_trigger_snare) {
            drum->snare_active = 1;
            drum->snare_envelope_time = 0.0f;
        }

        // Advance step counter (16 steps = 1 bar)
        drum->step_count++;
        if (drum->step_count >= 16) {
            drum->step_count = 0;
        }
    }

    // Generate kick - 909-style with pitch envelope
    float kick_out = 0.0f;
    if (drum->kick_active) {
        const float kick_duration = 0.4f;  // 400ms total

        if (drum->kick_envelope_time < kick_duration) {
            // Amplitude envelope - fast attack, exponential decay
            float amp_env = 1.0f - (drum->kick_envelope_time / kick_duration);
            amp_env = amp_env * amp_env * amp_env;  // Cubic decay

            // Pitch envelope - sweep from 180Hz down to 40Hz
            float pitch_env_time = drum->kick_envelope_time / 0.03f;  // 30ms pitch envelope
            if (pitch_env_time > 1.0f) pitch_env_time = 1.0f;
            pitch_env_time = pitch_env_time * pitch_env_time;  // Exponential curve

            float freq = 180.0f - (pitch_env_time * 140.0f);  // 180Hz -> 40Hz

            // Phase increment for oscillator
            float phase_inc = freq / (float)sample_rate;
            drum->osc_phase += phase_inc;
            if (drum->osc_phase >= 1.0f) drum->osc_phase -= 1.0f;

            // Simple sine approximation using parabola
            float t = drum->osc_phase * 2.0f - 1.0f;  // -1 to 1
            float sine;
            if (t >= 0.0f) {
                sine = 1.0f - 4.0f * (t - 0.5f) * (t - 0.5f);
            } else {
                sine = -(1.0f - 4.0f * (t + 0.5f) * (t + 0.5f));
            }

            // Apply amplitude envelope - boosted for louder bass
            kick_out = sine * amp_env * 1.5f;

            // Apply drive/saturation
            if (drum->drive_amount > 1.0f) {
                kick_out *= drum->drive_amount;
                // Soft clipping using tanh approximation
                // tanh(x) ≈ x / (1 + |x|) for a simpler soft clip
                if (kick_out > 0.0f) {
                    kick_out = kick_out / (1.0f + kick_out);
                } else {
                    kick_out = kick_out / (1.0f - kick_out);
                }
                kick_out *= 1.5f; // Compensate for clipping loss
            }

            drum->kick_envelope_time += 1.0f / (float)sample_rate;
        } else {
            drum->kick_active = 0;
        }
    }

    // Generate snare - 909-style with noise and envelope
    float snare_out = 0.0f;
    if (drum->snare_active) {
        const float snare_duration = 0.180f;  // 180ms total (like TR-909)

        if (drum->snare_envelope_time < snare_duration) {
            // Simple noise generator (Linear Congruential Generator)
            drum->noise_seed = drum->noise_seed * 1103515245u + 12345u;
            float noise = ((float)(drum->noise_seed >> 16) / 32768.0f) - 1.0f;

            // Amplitude envelope - exponential decay
            float amp_env = 1.0f - (drum->snare_envelope_time / snare_duration);
            amp_env = amp_env * amp_env;  // Quadratic decay for snappier sound

            // Apply envelope to noise
            snare_out = noise * amp_env * 0.6f;  // Scale down for mixing

            drum->snare_envelope_time += 1.0f / (float)sample_rate;
        } else {
            drum->snare_active = 0;
        }
    }

    // Mix kick and snare with input
    float drum_out = kick_out + snare_out;
    *out_l = (*in_l) * drum->input_mix + drum_out * drum->kick_mix;
    *out_r = (*in_r) * drum->input_mix + drum_out * drum->kick_mix;
}
