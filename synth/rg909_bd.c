/*
 * RG909 Bass Drum - Full TR-909 BD Synthesis with Sweep-Shape  
 * Circuit-accurate implementation with all authentic parameters
 */

#include "rg909_bd.h"
#include <math.h>
#include <string.h>
#include <stdio.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

void rg909_bd_init(RG909_BD* bd) {
    memset(bd, 0, sizeof(RG909_BD));
    
    // Basic parameters
    bd->level = 0.96f;
    bd->tune = 0.5f;
    bd->decay = 0.13f;
    bd->attack = 0.0f;
    
    // Sweep-shape timing (user-optimized defaults from original)
    bd->squiggly_end_ms = 1.5f;
    bd->fast_end_ms = 10.1f;
    bd->slow_end_ms = 31.65f;
    bd->tail_slow_start_ms = 74.0f;
    
    // Sweep-shape frequencies
    bd->squiggly_freq = 230.0f;
    bd->fast_freq = 216.0f;
    bd->slow_freq = 159.0f;
    bd->tail_freq = 88.0f;
    bd->tail_slow_freq = 53.0f;
    
    // SAW widths
    bd->fast_saw_pct = 14.2f;
    bd->slow_saw_pct = 6.0f;
    
    // Reset state
    bd->phase_offset = -1.0f;
    bd->tail_phase_offset = -1.0f;
}

void rg909_bd_reset(RG909_BD* bd) {
    bd->phase = 0.0f;
    bd->sweep_pos = 0.0f;
    bd->phase_offset = -1.0f;
    bd->tail_phase_offset = -1.0f;
    bd->active = 0;
}

void rg909_bd_trigger(RG909_BD* bd, uint8_t velocity, float sample_rate) {
    (void)velocity;
    (void)sample_rate;
    
    bd->sweep_pos = 0.0f;
    bd->phase = 0.0f;
    bd->phase_offset = -1.0f;
    bd->tail_phase_offset = -1.0f;
    bd->active = 1;
}

void rg909_bd_set_level(RG909_BD* bd, float level) {
    bd->level = level;
}

void rg909_bd_set_tune(RG909_BD* bd, float tune) {
    bd->tune = tune;
}

void rg909_bd_set_decay(RG909_BD* bd, float decay) {
    bd->decay = decay;
}

void rg909_bd_set_attack(RG909_BD* bd, float attack) {
    bd->attack = attack;
}

void rg909_bd_set_sweep_shape_timing(RG909_BD* bd,
    float squiggly_end_ms, float fast_end_ms,
    float slow_end_ms, float tail_slow_start_ms) {
    bd->squiggly_end_ms = squiggly_end_ms;
    bd->fast_end_ms = fast_end_ms;
    bd->slow_end_ms = slow_end_ms;
    bd->tail_slow_start_ms = tail_slow_start_ms;
}

void rg909_bd_set_sweep_shape_freqs(RG909_BD* bd,
    float squiggly_freq, float fast_freq, float slow_freq,
    float tail_freq, float tail_slow_freq) {
    bd->squiggly_freq = squiggly_freq;
    bd->fast_freq = fast_freq;
    bd->slow_freq = slow_freq;
    bd->tail_freq = tail_freq;
    bd->tail_slow_freq = tail_slow_freq;
}

void rg909_bd_set_sweep_shape_saw(RG909_BD* bd,
    float fast_saw_pct, float slow_saw_pct) {
    bd->fast_saw_pct = fast_saw_pct;
    bd->slow_saw_pct = slow_saw_pct;
}

float rg909_bd_process(RG909_BD* bd, float sample_rate) {
    if (!bd->active) {
        return 0.0f;
    }
    
    // Convert timing parameters from ms to seconds
    float squiggly_end = bd->squiggly_end_ms / 1000.0f;
    float fast_end = bd->fast_end_ms / 1000.0f;
    float slow_end = bd->slow_end_ms / 1000.0f;
    float tail_slow_start = bd->tail_slow_start_ms / 1000.0f;
    
    // Calculate phase increment based on current phase
    float phase_inc;
    float freq;
    
    if (bd->sweep_pos < squiggly_end) {
        freq = bd->squiggly_freq;
    } else if (bd->sweep_pos < fast_end) {
        freq = bd->fast_freq;
    } else if (bd->sweep_pos < slow_end) {
        freq = bd->slow_freq;
    } else if (bd->sweep_pos < tail_slow_start) {
        freq = bd->tail_freq;
    } else {
        freq = bd->tail_slow_freq;
    }
    
    phase_inc = freq / sample_rate;
    
    // Continue phase accumulator throughout all phases
    bd->phase += phase_inc;
    if (bd->phase >= 1.0f) bd->phase -= 1.0f;
    
    float sample = 0.0f;
    
    // TR-909 Waveform with Sweep-Shape Transient
    if (bd->sweep_pos < squiggly_end) {
        // Phase 1: Two-stage squiggly within 1.5ms
        float gradual_phase_end = squiggly_end * 0.67f;
        
        if (bd->sweep_pos < gradual_phase_end) {
            // Stage 1: Gradual squiggly rise
            float sine_val = sinf(2.0f * M_PI * bd->phase);
            sample = sine_val;
            
            float t = bd->sweep_pos / gradual_phase_end;
            float amp_env = 0.18f * powf(t, 0.8f);
            sample = sample * amp_env;
        } else {
            // Stage 2: Steep rise/punch
            float sine_val = sinf(2.0f * M_PI * bd->phase);
            sample = fabsf(sine_val);  // Full-wave rectification
            
            sample = tanhf(sample * 1.3f);
            
            float rise_duration = squiggly_end - gradual_phase_end;
            float t = (bd->sweep_pos - gradual_phase_end) / rise_duration;
            
            float amp_env = 0.18f + (0.97f - 0.18f) * powf(t, 2.5f);
            sample = sample * amp_env;
        }
        
    } else if (bd->sweep_pos < slow_end) {
        // Phase 2: Sweep-shape
        int is_slow_sweep = (bd->sweep_pos >= fast_end);
        
        // Capture phase offset on first entry
        if (bd->phase_offset < 0.0f) {
            bd->phase_offset = bd->phase;
        }
        
        float u = fmodf(bd->phase - bd->phase_offset + 10.0f, 1.0f);
        
        // Sweep-shape waveform (SAW → COSINE down → SAW → COSINE up)
        float saw_width = is_slow_sweep ? (bd->slow_saw_pct / 100.0f) : (bd->fast_saw_pct / 100.0f);
        float cosine_width = (0.5f - saw_width);
        
        if (u < saw_width) {
            // Quarter 1: SAW fade (top)
            float t_quarter = u / saw_width;
            sample = 0.90f + (0.85f - 0.90f) * t_quarter;
        } else if (u < 0.5f) {
            // Quarter 2: COSINE sweep DOWN
            float t_quarter = (u - saw_width) / cosine_width;
            float c = cosf(t_quarter * M_PI);
            sample = 0.5f * (0.85f + (-0.85f)) + 0.5f * (0.85f - (-0.85f)) * c;
        } else if (u < 0.5f + saw_width) {
            // Quarter 3: SAW fade (bottom)
            float t_quarter = (u - 0.5f) / saw_width;
            sample = -0.85f + ((-0.80f) - (-0.85f)) * t_quarter;
        } else {
            // Quarter 4: COSINE sweep UP
            float t_quarter = (u - 0.5f - saw_width) / cosine_width;
            float c = cosf((1.0f - t_quarter) * M_PI);
            sample = 0.5f * ((-0.80f) + 0.90f) + 0.5f * (0.90f - (-0.80f)) * c;
        }
        
    } else {
        // Phase 3 & 4: Tail with triangular-sine
        float tri_phase;
        float phase_invert;
        
        if (bd->sweep_pos < tail_slow_start) {
            // Phase 3: Tail (normal)
            phase_invert = 1.0f;
            
            if (bd->tail_phase_offset < 0.0f) {
                bd->tail_phase_offset = bd->phase - 0.25f;
            }
            
            tri_phase = fmodf(bd->phase - bd->tail_phase_offset + 10.0f, 1.0f);
            
        } else {
            // Phase 4: Tail slow (inverted)
            phase_invert = -1.0f;
            
            float base_tri_phase = fmodf(bd->phase - bd->tail_phase_offset + 10.0f, 1.0f);
            tri_phase = fmodf(base_tri_phase + 0.5f, 1.0f);
        }
        
        // Generate triangular-sine waveform
        float triangle = 4.0f * fabsf(tri_phase - 0.5f) - 1.0f;
        float clipped = tanhf(triangle * 1.5f);
        sample = (triangle * 0.20f + clipped * 0.80f) * phase_invert;
        
        // Two-stage decay envelope
        float amp_env;
        float sustain_end = slow_end + 0.0085f;
        if (bd->sweep_pos < sustain_end) {
            float t = (bd->sweep_pos - slow_end) / 0.0085f;
            if (t < 0.0f) t = 0.0f;
            if (t > 1.0f) t = 1.0f;
            amp_env = 0.98f - (0.10f * t);
        } else {
            float t_decay = bd->sweep_pos - sustain_end;
            float decay_time = 0.045f + bd->decay * 0.085f;
            float k = 0.6f / decay_time;
            amp_env = 0.88f * expf(-k * t_decay);
            
            if (amp_env < 0.0001f) {
                bd->active = 0;
            }
        }
        
        sample = sample * amp_env;
    }
    
    // Output gain - match real 909 RMS level
    sample = sample * 2.0f;
    
    bd->sweep_pos += 1.0f / sample_rate;
    sample *= bd->level;
    
    return sample;
}
