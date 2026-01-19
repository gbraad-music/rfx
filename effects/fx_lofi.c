/**
 * FX Lofi Effect - Implementation
 */

#include "fx_lofi.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// Soft clipping (tanh approximation)
static inline float soft_clip(float x, float amount) {
    if (amount <= 0.0f) return x;

    float drive = 1.0f + amount * 4.0f;
    x *= drive;

    // Fast tanh approximation
    float x2 = x * x;
    return x * (27.0f + x2) / (27.0f + 9.0f * x2);
}

// Simple white noise generator (xorshift)
static inline float white_noise(uint32_t* seed) {
    *seed ^= *seed << 13;
    *seed ^= *seed >> 17;
    *seed ^= *seed << 5;
    return ((float)(*seed) / (float)UINT32_MAX) * 2.0f - 1.0f;
}

// ============================================================================
// Lifecycle
// ============================================================================

FX_Lofi* fx_lofi_create(uint32_t sample_rate) {
    FX_Lofi* lofi = (FX_Lofi*)calloc(1, sizeof(FX_Lofi));
    if (!lofi) return NULL;

    lofi->sample_rate = sample_rate;

    // Default parameters (clean/bypass)
    lofi->enabled = false;  // Disabled by default
    lofi->bit_depth = 16.0f;
    lofi->sample_rate_ratio = 1.0f;
    lofi->filter_cutoff = 20000.0f;
    lofi->saturation = 0.0f;
    lofi->noise_level = 0.0f;
    lofi->wow_flutter_depth = 0.0f;
    lofi->wow_flutter_rate = 0.5f;

    // Internal state
    lofi->downsample_phase = 0.0f;
    lofi->last_output[0] = 0.0f;
    lofi->last_output[1] = 0.0f;
    lofi->filter_state[0] = 0.0f;
    lofi->filter_state[1] = 0.0f;
    lofi->lfo_phase = 0.0f;
    lofi->noise_seed = 12345;

    return lofi;
}

void fx_lofi_destroy(FX_Lofi* lofi) {
    if (lofi) free(lofi);
}

void fx_lofi_reset(FX_Lofi* lofi) {
    if (!lofi) return;

    lofi->downsample_phase = 0.0f;
    lofi->last_output[0] = 0.0f;
    lofi->last_output[1] = 0.0f;
    lofi->filter_state[0] = 0.0f;
    lofi->filter_state[1] = 0.0f;
    lofi->lfo_phase = 0.0f;
}

// ============================================================================
// Parameters
// ============================================================================

void fx_lofi_set_enabled(FX_Lofi* lofi, bool enabled) {
    if (!lofi) return;
    lofi->enabled = enabled;
}

bool fx_lofi_get_enabled(FX_Lofi* lofi) {
    return lofi ? lofi->enabled : false;
}

void fx_lofi_set_bit_depth(FX_Lofi* lofi, float normalized) {
    if (!lofi) return;
    // Map to discrete bit depths: 2, 8, 12, 16 (like AKAI/Amiga)
    normalized = fmaxf(0.0f, fminf(1.0f, normalized));

    if (normalized < 0.25f) {
        lofi->bit_depth = 2.0f;   // 0-24%: 2-bit
    } else if (normalized < 0.5f) {
        lofi->bit_depth = 8.0f;   // 25-49%: 8-bit
    } else if (normalized < 0.75f) {
        lofi->bit_depth = 12.0f;  // 50-74%: 12-bit
    } else {
        lofi->bit_depth = 16.0f;  // 75-100%: 16-bit (clean)
    }
}

void fx_lofi_set_sample_rate_ratio(FX_Lofi* lofi, float normalized) {
    if (!lofi) return;
    // Map to discrete sample rates: Amiga + AKAI vintage rates
    normalized = fmaxf(0.0f, fminf(1.0f, normalized));

    if (normalized < 0.125f) {
        lofi->sample_rate_ratio = 7500.0f / 48000.0f;   // 0-12%: 7.5kHz (AKAI S950 lowest)
    } else if (normalized < 0.25f) {
        lofi->sample_rate_ratio = 8363.0f / 48000.0f;   // 12-25%: 8363 Hz (Amiga Paula - most common!)
    } else if (normalized < 0.375f) {
        lofi->sample_rate_ratio = 10000.0f / 48000.0f;  // 25-37%: 10kHz (AKAI S950)
    } else if (normalized < 0.5f) {
        lofi->sample_rate_ratio = 15000.0f / 48000.0f;  // 37-50%: 15kHz (AKAI S950)
    } else if (normalized < 0.625f) {
        lofi->sample_rate_ratio = 16726.0f / 48000.0f;  // 50-62%: 16726 Hz (Amiga Paula 2x)
    } else if (normalized < 0.75f) {
        lofi->sample_rate_ratio = 22050.0f / 48000.0f;  // 62-75%: 22.05kHz (AKAI/Standard)
    } else if (normalized < 0.875f) {
        lofi->sample_rate_ratio = 32000.0f / 48000.0f;  // 75-87%: 32kHz
    } else {
        lofi->sample_rate_ratio = 1.0f;                 // 87-100%: 48kHz (NO reduction - clean!)
    }
}

void fx_lofi_set_filter_cutoff(FX_Lofi* lofi, float normalized) {
    if (!lofi) return;
    // Map 0-1 to 200Hz-20kHz (logarithmic)
    normalized = fmaxf(0.0f, fminf(1.0f, normalized));
    float log_min = logf(200.0f);
    float log_max = logf(20000.0f);
    lofi->filter_cutoff = expf(log_min + normalized * (log_max - log_min));
}

void fx_lofi_set_saturation(FX_Lofi* lofi, float normalized) {
    if (!lofi) return;
    // Map 0-1 to 0-2.0
    normalized = fmaxf(0.0f, fminf(1.0f, normalized));
    lofi->saturation = normalized * 2.0f;
}

void fx_lofi_set_noise_level(FX_Lofi* lofi, float normalized) {
    if (!lofi) return;
    // Already 0-1 range
    lofi->noise_level = fmaxf(0.0f, fminf(1.0f, normalized));
}

void fx_lofi_set_wow_flutter_depth(FX_Lofi* lofi, float normalized) {
    if (!lofi) return;
    // Already 0-1 range
    lofi->wow_flutter_depth = fmaxf(0.0f, fminf(1.0f, normalized));
}

void fx_lofi_set_wow_flutter_rate(FX_Lofi* lofi, float normalized) {
    if (!lofi) return;
    // Map 0-1 to 0.1-10 Hz
    normalized = fmaxf(0.0f, fminf(1.0f, normalized));
    lofi->wow_flutter_rate = 0.1f + normalized * 9.9f;
}

float fx_lofi_get_bit_depth(FX_Lofi* lofi) {
    if (!lofi) return 1.0f;  // Default: 100% = 16-bit

    // Return normalized 0-1 value (reverse of setter mapping)
    if (lofi->bit_depth <= 2.0f) {
        return 0.125f;  // Middle of 0-25% range
    } else if (lofi->bit_depth <= 8.0f) {
        return 0.375f;  // Middle of 25-50% range
    } else if (lofi->bit_depth <= 12.0f) {
        return 0.625f;  // Middle of 50-75% range
    } else {
        return 1.0f;    // 75-100% range
    }
}

float fx_lofi_get_sample_rate_ratio(FX_Lofi* lofi) {
    if (!lofi) return 1.0f;  // Default: 100% = no reduction

    // Return normalized 0-1 value (reverse of 8-step setter mapping)
    float ratio = lofi->sample_rate_ratio;
    if (ratio <= 7500.0f / 48000.0f + 0.001f) {
        return 0.0625f;  // Middle of 0-12.5% range (7.5kHz AKAI)
    } else if (ratio <= 8363.0f / 48000.0f + 0.001f) {
        return 0.1875f;  // Middle of 12.5-25% range (8363 Hz Amiga)
    } else if (ratio <= 10000.0f / 48000.0f + 0.001f) {
        return 0.3125f;  // Middle of 25-37.5% range (10kHz AKAI)
    } else if (ratio <= 15000.0f / 48000.0f + 0.001f) {
        return 0.4375f;  // Middle of 37.5-50% range (15kHz AKAI)
    } else if (ratio <= 16726.0f / 48000.0f + 0.001f) {
        return 0.5625f;  // Middle of 50-62.5% range (16726 Hz Amiga 2x)
    } else if (ratio <= 22050.0f / 48000.0f + 0.001f) {
        return 0.6875f;  // Middle of 62.5-75% range (22.05kHz)
    } else if (ratio <= 32000.0f / 48000.0f + 0.001f) {
        return 0.8125f;  // Middle of 75-87.5% range (32kHz)
    } else {
        return 0.9375f;  // Middle of 87.5-100% range (48kHz clean)
    }
}

float fx_lofi_get_filter_cutoff(FX_Lofi* lofi) {
    if (!lofi) return 1.0f;  // Default: 100% = 20kHz (no filtering)

    // Return normalized 0-1 value (reverse of logarithmic setter mapping)
    float cutoff = lofi->filter_cutoff;
    float log_min = logf(200.0f);
    float log_max = logf(20000.0f);
    float log_cutoff = logf(cutoff);

    return (log_cutoff - log_min) / (log_max - log_min);
}

float fx_lofi_get_saturation(FX_Lofi* lofi) {
    if (!lofi) return 0.0f;  // Default: 0

    // Return normalized 0-1 value (reverse of setter: saturation = normalized * 2.0)
    return lofi->saturation / 2.0f;
}

float fx_lofi_get_noise_level(FX_Lofi* lofi) {
    return lofi ? lofi->noise_level : 0.0f;
}

float fx_lofi_get_wow_flutter_depth(FX_Lofi* lofi) {
    return lofi ? lofi->wow_flutter_depth : 0.0f;
}

float fx_lofi_get_wow_flutter_rate(FX_Lofi* lofi) {
    if (!lofi) return 0.5f;  // Default: 50% = 5Hz

    // Return normalized 0-1 value (reverse of setter: rate = 0.1 + normalized * 9.9)
    return (lofi->wow_flutter_rate - 0.1f) / 9.9f;
}

// ============================================================================
// Processing
// ============================================================================

void fx_lofi_process_f32(FX_Lofi* lofi, float* buffer, uint32_t frames, uint32_t sample_rate) {
    if (!lofi || !buffer) return;

    // Use passed sample_rate instead of stored one (for offline rendering)
    (void)sample_rate;  // Suppress unused warning for now

    // Bypass if disabled
    if (!lofi->enabled) {
        return;
    }

    // Calculate filter coefficient (one-pole low-pass)
    float filter_coeff = expf(-2.0f * M_PI * lofi->filter_cutoff / lofi->sample_rate);

    // Bit depth quantization - explicit lookup to avoid any pow() issues
    int num_levels;
    if (lofi->bit_depth <= 2.0f) {
        num_levels = 4;        // 2-bit
    } else if (lofi->bit_depth <= 8.0f) {
        num_levels = 256;      // 8-bit
    } else if (lofi->bit_depth <= 12.0f) {
        num_levels = 4096;     // 12-bit
    } else {
        num_levels = 65536;    // 16-bit
    }

    // Sample rate reduction increment
    float downsample_inc = lofi->sample_rate_ratio;

    // LFO increment for wow/flutter
    float lfo_inc = 2.0f * M_PI * lofi->wow_flutter_rate / lofi->sample_rate;

    for (uint32_t i = 0; i < frames; i++) {
        float left = buffer[i * 2];
        float right = buffer[i * 2 + 1];

        // === LOW-PASS FILTER (apply BEFORE bit reduction to anti-alias) ===
        if (lofi->filter_cutoff < 20000.0f) {
            lofi->filter_state[0] = lofi->filter_state[0] * filter_coeff + left * (1.0f - filter_coeff);
            lofi->filter_state[1] = lofi->filter_state[1] * filter_coeff + right * (1.0f - filter_coeff);
            left = lofi->filter_state[0];
            right = lofi->filter_state[1];
        }

        // === SATURATION ===
        if (lofi->saturation > 0.0f) {
            left = soft_clip(left, lofi->saturation);
            right = soft_clip(right, lofi->saturation);
        }

        // === SAMPLE RATE REDUCTION ===
        if (lofi->sample_rate_ratio < 1.0f) {
            lofi->downsample_phase += downsample_inc;

            if (lofi->downsample_phase >= 1.0f) {
                lofi->downsample_phase -= 1.0f;
                lofi->last_output[0] = left;
                lofi->last_output[1] = right;
            }

            left = lofi->last_output[0];
            right = lofi->last_output[1];
        }

        // === BIT REDUCTION (apply LAST for maximum effect) ===
        // Check num_levels instead of bit_depth (more reliable)
        if (num_levels < 65536) {
            // Map -1..+1 to 0..1
            float left_01 = (left + 1.0f) * 0.5f;
            float right_01 = (right + 1.0f) * 0.5f;

            // Quantize to integer levels 0..(num_levels-1)
            int left_int = (int)(left_01 * (float)(num_levels - 1) + 0.5f);
            int right_int = (int)(right_01 * (float)(num_levels - 1) + 0.5f);

            // Clamp
            if (left_int < 0) left_int = 0;
            if (left_int >= num_levels) left_int = num_levels - 1;
            if (right_int < 0) right_int = 0;
            if (right_int >= num_levels) right_int = num_levels - 1;

            // Map back: int level → 0..1 → -1..+1
            left = ((float)left_int / (float)(num_levels - 1)) * 2.0f - 1.0f;
            right = ((float)right_int / (float)(num_levels - 1)) * 2.0f - 1.0f;
        }

        // === NOISE ===
        if (lofi->noise_level > 0.0f) {
            float noise = white_noise(&lofi->noise_seed) * lofi->noise_level * 0.05f;
            left += noise;
            right += noise;
        }

        // === WOW/FLUTTER (simple amplitude modulation for now) ===
        if (lofi->wow_flutter_depth > 0.0f) {
            float lfo = sinf(lofi->lfo_phase) * lofi->wow_flutter_depth * 0.1f;
            left *= (1.0f + lfo);
            right *= (1.0f + lfo);

            lofi->lfo_phase += lfo_inc;
            if (lofi->lfo_phase >= 2.0f * M_PI) {
                lofi->lfo_phase -= 2.0f * M_PI;
            }
        }

        // Clamp output
        left = fmaxf(-1.0f, fminf(1.0f, left));
        right = fmaxf(-1.0f, fminf(1.0f, right));

        buffer[i * 2] = left;
        buffer[i * 2 + 1] = right;
    }
}
