/*
 * RG909 Bass Drum - Modular TR-909 BD Synthesis
 * Extracted from rg909_drum_synth.c for standalone use
 */

#ifndef RG909_BD_H
#define RG909_BD_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    // Voice state
    float sweep_pos;        // Time counter in seconds
    float phase;            // Phase accumulator (0-1)
    float phase_offset;     // Phase offset for sweep-shape
    float tail_phase_offset; // Phase offset for tail
    int active;

    // Parameters (0.0-1.0 range for basic params, except level which can exceed 1.0)
    float level;
    float tune;
    float decay;
    float attack;

    // Sweep-shape timing parameters (milliseconds)
    float squiggly_end_ms;
    float fast_end_ms;
    float slow_end_ms;
    float tail_slow_start_ms;

    // Sweep-shape frequency parameters (Hz)
    float squiggly_freq;
    float fast_freq;
    float slow_freq;
    float tail_freq;
    float tail_slow_freq;

    // Sweep-shape SAW width parameters (percentage, 0-100)
    float fast_saw_pct;
    float slow_saw_pct;
} RG909_BD;

// Lifecycle
void rg909_bd_init(RG909_BD* bd);
void rg909_bd_reset(RG909_BD* bd);

// Trigger
void rg909_bd_trigger(RG909_BD* bd, uint8_t velocity, float sample_rate);

// Processing
float rg909_bd_process(RG909_BD* bd, float sample_rate);

// Basic parameters (0.0-1.0 range)
void rg909_bd_set_level(RG909_BD* bd, float level);
void rg909_bd_set_tune(RG909_BD* bd, float tune);
void rg909_bd_set_decay(RG909_BD* bd, float decay);
void rg909_bd_set_attack(RG909_BD* bd, float attack);

// Sweep-shape parameters (advanced)
void rg909_bd_set_sweep_shape_timing(RG909_BD* bd,
    float squiggly_end_ms, float fast_end_ms,
    float slow_end_ms, float tail_slow_start_ms);

void rg909_bd_set_sweep_shape_freqs(RG909_BD* bd,
    float squiggly_freq, float fast_freq, float slow_freq,
    float tail_freq, float tail_slow_freq);

void rg909_bd_set_sweep_shape_saw(RG909_BD* bd,
    float fast_saw_pct, float slow_saw_pct);

#ifdef __cplusplus
}
#endif

#endif // RG909_BD_H
