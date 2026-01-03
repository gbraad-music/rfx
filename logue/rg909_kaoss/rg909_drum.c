#include "rg909_drum.h"
#include "rg909_bd.h"
#include <stdlib.h>

struct RG909Drum {
    float global_tempo_bpm;
    float kick_density;      // 0.0 = no kicks, 1.0 = max kick variation
    float snare_variation;   // 0.0 = no snare, 1.0 = max snare variation
    float tone_amount;       // Kick tone/pitch
    float drum_mix;
    float input_mix;

    float metro_phase;
    float metro_increment;
    int step_count;       // 16th note counter (0-15, one bar)

    // Kick state (using RG909_BD)
    RG909_BD bd_;

    // Snare state
    int snare_active;
    float snare_envelope_time;
    unsigned int noise_seed;  // For simple noise generation
};

RG909Drum* rg909_drum_create(void)
{
    RG909Drum* drum = (RG909Drum*)malloc(sizeof(RG909Drum));
    if (!drum) return NULL;

    drum->global_tempo_bpm = 120.0f;
    drum->kick_density = 0.5f;      // Some kicks by default
    drum->snare_variation = 0.0f;   // No snare by default
    drum->tone_amount = 0.5f;       // Medium tone
    drum->drum_mix = 0.9f;
    drum->input_mix = 0.1f;

    drum->metro_phase = 0.0f;
    drum->metro_increment = 0.0f;
    drum->step_count = 0;

    // Initialize RG909 BD
    rg909_bd_init(&drum->bd_);
    rg909_bd_set_level(&drum->bd_, 0.8f);
    rg909_bd_set_tune(&drum->bd_, 0.5f);
    rg909_bd_set_decay(&drum->bd_, 0.5f);
    rg909_bd_set_attack(&drum->bd_, 0.0f);

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

void rg909_drum_destroy(RG909Drum* drum)
{
    if (drum) {
        free(drum);
    }
}

void rg909_drum_set_kick_density(RG909Drum* drum, float density)
{
    if (!drum) return;
    drum->kick_density = density;
}

void rg909_drum_set_snare_variation(RG909Drum* drum, float variation)
{
    if (!drum) return;
    drum->snare_variation = variation;
}

void rg909_drum_set_tone(RG909Drum* drum, float tone)
{
    if (!drum) return;
    drum->tone_amount = tone;
    rg909_bd_set_tune(&drum->bd_, tone);
}

void rg909_drum_set_mix(RG909Drum* drum, float mix)
{
    if (!drum) return;
    drum->drum_mix = mix;
}

void rg909_drum_set_tempo(RG909Drum* drum, float bpm)
{
    if (!drum) return;
    drum->global_tempo_bpm = bpm;

    if (bpm < 30.0f) bpm = 30.0f;
    if (bpm > 300.0f) bpm = 300.0f;
    float beats_per_second = bpm / 60.0f;
    float sixteenths_per_second = beats_per_second * 4.0f;
    drum->metro_increment = sixteenths_per_second / 48000.0f;
}

void rg909_drum_process(RG909Drum* drum, const float* in_l, const float* in_r,
                        float* out_l, float* out_r, int sample_rate)
{
    if (!drum) return;

    // Metro - runs at 16th note resolution
    drum->metro_phase += drum->metro_increment;
    if (drum->metro_phase >= 1.0f) {
        drum->metro_phase -= 1.0f;

        // Kick triggers - based on kick_density
        int should_trigger_kick = 0;

        if (drum->kick_density > 0.01f) {
            // Main beats: steps 0, 4, 8, 12 (quarter notes = 4-on-floor)
            if (drum->step_count == 0 || drum->step_count == 4 ||
                drum->step_count == 8 || drum->step_count == 12) {
                should_trigger_kick = 1;
            }

            // Add syncopation based on kick_density
            if (drum->kick_density > 0.2f) {
                if (drum->step_count == 14) {
                    should_trigger_kick = 1;
                }
            }

            if (drum->kick_density > 0.5f) {
                if (drum->step_count == 6) {
                    should_trigger_kick = 1;
                }
            }

            if (drum->kick_density > 0.8f) {
                if (drum->step_count == 10) {
                    should_trigger_kick = 1;
                }
            }
        }

        if (should_trigger_kick) {
            rg909_bd_trigger(&drum->bd_, 127, sample_rate);
        }

        // Snare triggers - based on snare_variation
        int should_trigger_snare = 0;

        if (drum->snare_variation > 0.01f) {
            // Backbeats: steps 4, 12 (beats 2 and 4 - classic snare pattern)
            if (drum->step_count == 4 || drum->step_count == 12) {
                should_trigger_snare = 1;
            }

            // Add variation based on snare_variation
            if (drum->snare_variation > 0.3f) {
                if (drum->step_count == 2) {
                    should_trigger_snare = 1;
                }
            }

            if (drum->snare_variation > 0.6f) {
                if (drum->step_count == 10) {
                    should_trigger_snare = 1;
                }
            }

            if (drum->snare_variation > 0.85f) {
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

    // Generate kick using RG909_BD
    float kick_out = rg909_bd_process(&drum->bd_, sample_rate);

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
    *out_l = (*in_l) * drum->input_mix + drum_out * drum->drum_mix;
    *out_r = (*in_r) * drum->input_mix + drum_out * drum->drum_mix;
}
