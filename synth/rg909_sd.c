/*
 * RG909 Snare Drum - Full Waldorf TR-909 SD Architecture
 * Dual resonators + dual noise paths (LP always-on + HP envelope)
 */

#include "rg909_sd.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdio.h>

void rg909_sd_init(RG909_SD* sd) {
    memset(sd, 0, sizeof(RG909_SD));

    // Create synth components
    sd->res1 = synth_resonator_create();
    sd->res2 = synth_resonator_create();
    sd->noise = synth_noise_create();
    sd->filter = synth_filter_create();

    // Basic parameters (matching original defaults)
    sd->level = 44.0f;         // Master output level
    sd->tone = 0.01f;          // High-pass filter cutoff (VERY LOW)
    sd->snappy = 0.0115f;      // HP noise level
    sd->tuning = 0.5f;
    sd->tone_gain = 0.5f;      // Resonator output gain
    
    // Waldorf architecture parameters (user-tweaked)
    sd->freq1 = 120.0f;        // Osc 1: Constant frequency (DOMINANT)
    sd->freq2 = 122.0f;        // Osc 2: Swept frequency (MINIMAL)
    sd->res1_level = 8.5f;     // Osc 1 strike level (STRONG)
    sd->res2_level = 1.5f;     // Osc 2 strike level (WEAK)
    sd->noise_level = 0.0f;    // LP noise level (TODO)
    sd->lp_noise_cutoff = 0.15f; // LP noise filter cutoff
    sd->res1_decay = 0.46f;    // Osc 1 decay (LONG sustain)
    sd->res2_decay = 0.05f;    // Osc 2 decay (VERY SHORT)
    
    // Pitch envelope
    sd->sweep_amount = 1.8f;   // Pitch multiplier at start
    sd->sweep_time = 0.012f;   // 12ms fast pitch envelope
    sd->noise_decay = 0.180f;  // HP noise decay time
}

void rg909_sd_reset(RG909_SD* sd) {
    if (sd->res1) synth_resonator_reset(sd->res1);
    if (sd->res2) synth_resonator_reset(sd->res2);
    sd->sweep_pos = 0.0f;
    sd->noise_env = 0.0f;
    sd->decay_env = 0.0f;
    sd->active = 0;
}

void rg909_sd_trigger(RG909_SD* sd, uint8_t velocity, float sample_rate) {
    float vel = velocity / 127.0f;

    sd->sweep_pos = 0.0f;
    sd->active = 1;

    // Reset and configure resonators with pitch envelope
    synth_resonator_reset(sd->res1);
    synth_resonator_reset(sd->res2);

    // BOTH resonators start at swept-up frequency
    float freq1 = sd->freq1 * sd->sweep_amount;
    float freq2 = sd->freq2 * sd->sweep_amount;

    synth_resonator_set_params(sd->res1, freq1, sd->res1_decay, sample_rate);
    synth_resonator_set_params(sd->res2, freq2, sd->res2_decay, sample_rate);

    // Strike resonators
    synth_resonator_strike(sd->res1, vel * sd->res1_level);
    synth_resonator_strike(sd->res2, vel * sd->res2_level);

    // HP noise envelope
    sd->noise_env = vel * sd->snappy;

    // Configure HP filter for snappy noise
    synth_filter_set_type(sd->filter, SYNTH_FILTER_HPF);
    synth_filter_set_cutoff(sd->filter, 0.4f + sd->tone * 0.3f);
    synth_filter_set_resonance(sd->filter, 0.5f);

    // Initialize LP noise filter state
    sd->decay_env = 0.0f;
}

void rg909_sd_set_level(RG909_SD* sd, float level) {
    sd->level = level;
}

void rg909_sd_set_tone(RG909_SD* sd, float tone) {
    sd->tone = tone;
}

void rg909_sd_set_snappy(RG909_SD* sd, float snappy) {
    sd->snappy = snappy;
}

void rg909_sd_set_tuning(RG909_SD* sd, float tuning) {
    sd->tuning = tuning;
}

float rg909_sd_process(RG909_SD* sd, float sample_rate) {
    if (!sd->active) {
        return 0.0f;
    }

    // Pitch envelope (applied to BOTH resonators)
    float pitch_env = 1.0f;
    if (sd->sweep_pos < sd->sweep_time) {
        float t = sd->sweep_pos / sd->sweep_time;
        pitch_env = sd->sweep_amount - (sd->sweep_amount - 1.0f) * t;
    }

    // Update resonator frequencies with pitch envelope
    float freq1 = sd->freq1 * pitch_env;
    float freq2 = sd->freq2 * pitch_env;
    synth_resonator_set_params(sd->res1, freq1, sd->res1_decay, sample_rate);
    synth_resonator_set_params(sd->res2, freq2, sd->res2_decay, sample_rate);

    // Get tone from BOTH resonators
    float t1 = synth_resonator_process(sd->res1, 0.0f);
    float t2 = synth_resonator_process(sd->res2, 0.0f);
    float tone = (t1 + t2) * 0.5f * sd->tone_gain;

    // Generate raw noise
    float noise_raw = synth_noise_process(sd->noise);

    // NOISE PATH 1: Low-pass filtered, always present
    float lp_cutoff = sd->lp_noise_cutoff;
    sd->decay_env = sd->decay_env + lp_cutoff * (noise_raw - sd->decay_env);
    float noise_lp = sd->decay_env * sd->noise_level;

    // NOISE PATH 2: High-pass filtered, envelope controlled
    sd->noise_env -= sd->noise_env * (1.0f / (sd->noise_decay * sample_rate));
    if (sd->noise_env < 0.0001f) sd->noise_env = 0.0f;

    float noise_hp = synth_filter_process(sd->filter, noise_raw, sample_rate);
    noise_hp *= sd->noise_env;

    // Combine all components
    float sample = tone + noise_lp + noise_hp;

    // MASTER AMPLITUDE DECAY ENVELOPE
    float decay_time = 0.120f;  // Fixed 120ms master decay
    float amp_env = expf(-3.0f * sd->sweep_pos / decay_time);

    sample *= amp_env * sd->level;

    // Update time
    sd->sweep_pos += 1.0f / sample_rate;

    // Deactivate when envelope has decayed
    if (sd->sweep_pos > 0.300f || amp_env < 0.001f) {
        sd->active = 0;
    }

    return sample;
}

void rg909_sd_destroy(RG909_SD* sd) {
    if (sd->res1) {
        synth_resonator_destroy(sd->res1);
        sd->res1 = NULL;
    }
    if (sd->res2) {
        synth_resonator_destroy(sd->res2);
        sd->res2 = NULL;
    }
    if (sd->noise) {
        synth_noise_destroy(sd->noise);
        sd->noise = NULL;
    }
    if (sd->filter) {
        synth_filter_destroy(sd->filter);
        sd->filter = NULL;
    }
}
