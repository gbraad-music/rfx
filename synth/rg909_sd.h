/*
 * RG909 Snare Drum - Full Waldorf TR-909 SD Architecture
 * Uses resonators, filters, and noise modules
 */

#ifndef RG909_SD_H
#define RG909_SD_H

#include <stdint.h>
#include "synth_resonator.h"
#include "synth_noise.h"
#include "synth_filter.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    // Waldorf architecture components
    SynthResonator* res1;      // Primary resonator (constant pitch)
    SynthResonator* res2;      // Secondary resonator (pitch modulated)
    SynthNoise* noise;
    SynthFilter* filter;       // High-pass filter for snappy noise

    // Voice state
    float sweep_pos;           // Time counter
    float sweep_time;          // Pitch envelope time
    float sweep_amount;        // Pitch envelope amount
    float noise_env;           // HP noise envelope
    float noise_decay;         // HP noise decay time
    float decay_env;           // Reused as LP filter state
    int active;

    // Parameters
    float level;               // Master output level
    float tone;                // High-pass filter cutoff
    float snappy;              // HP noise level
    float tuning;              // Resonator tuning
    float tone_gain;           // Resonator output gain

    // Extended parameters (Waldorf architecture)
    float freq1;               // Osc 1 frequency (Hz)
    float freq2;               // Osc 2 frequency (Hz)
    float res1_level;          // Osc 1 strike level
    float res2_level;          // Osc 2 strike level
    float noise_level;         // LP noise level
    float lp_noise_cutoff;     // LP noise filter cutoff
    float res1_decay;          // Osc 1 decay time (seconds)
    float res2_decay;          // Osc 2 decay time (seconds)
} RG909_SD;

// Lifecycle
void rg909_sd_init(RG909_SD* sd);
void rg909_sd_destroy(RG909_SD* sd);
void rg909_sd_reset(RG909_SD* sd);

// Trigger
void rg909_sd_trigger(RG909_SD* sd, uint8_t velocity, float sample_rate);

// Processing
float rg909_sd_process(RG909_SD* sd, float sample_rate);

// Parameters (0.0-1.0 range)
void rg909_sd_set_level(RG909_SD* sd, float level);
void rg909_sd_set_tone(RG909_SD* sd, float tone);
void rg909_sd_set_snappy(RG909_SD* sd, float snappy);
void rg909_sd_set_tuning(RG909_SD* sd, float tuning);

#ifdef __cplusplus
}
#endif

#endif // RG909_SD_H
