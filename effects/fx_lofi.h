/**
 * FX Lofi Effect - Bit Crushing, Sample Rate Reduction, and Degradation
 * Provides vintage/degraded sound aesthetics
 */

#ifndef FX_LOFI_H
#define FX_LOFI_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct FX_Lofi {
    // Parameters
    bool enabled;              // Effect on/off
    float bit_depth;           // 1.0-16.0 bits (16.0 = clean)
    float sample_rate_ratio;   // 0.1-1.0 (1.0 = no reduction)
    float filter_cutoff;       // 200-20000 Hz (low-pass for muffled sound)
    float saturation;          // 0.0-2.0 (soft clipping amount)
    float noise_level;         // 0.0-1.0 (tape/vinyl noise)
    float wow_flutter_depth;   // 0.0-1.0 (pitch modulation depth)
    float wow_flutter_rate;    // 0.1-10.0 Hz (pitch modulation speed)

    // Internal state
    uint32_t sample_rate;
    float downsample_phase;    // For sample rate reduction
    float last_output[2];      // Hold for sample rate reduction (L/R)

    // Low-pass filter state (simple one-pole)
    float filter_state[2];     // L/R

    // Wow/flutter LFO phase
    float lfo_phase;

    // Noise generator state
    uint32_t noise_seed;
} FX_Lofi;

// ============================================================================
// Lifecycle
// ============================================================================

/**
 * Create Lofi effect instance
 */
FX_Lofi* fx_lofi_create(uint32_t sample_rate);

/**
 * Destroy Lofi effect instance
 */
void fx_lofi_destroy(FX_Lofi* lofi);

/**
 * Reset effect state
 */
void fx_lofi_reset(FX_Lofi* lofi);

// ============================================================================
// Parameters
// ============================================================================

/**
 * Enable/disable effect
 */
void fx_lofi_set_enabled(FX_Lofi* lofi, bool enabled);
bool fx_lofi_get_enabled(FX_Lofi* lofi);

/**
 * Set bit depth (1.0-16.0, default 16.0 = clean)
 */
void fx_lofi_set_bit_depth(FX_Lofi* lofi, float bits);

/**
 * Set sample rate reduction ratio (0.1-1.0, default 1.0 = no reduction)
 * 0.5 = half sample rate, 0.25 = quarter sample rate, etc.
 */
void fx_lofi_set_sample_rate_ratio(FX_Lofi* lofi, float ratio);

/**
 * Set low-pass filter cutoff (200-20000 Hz, default 20000 = no filtering)
 */
void fx_lofi_set_filter_cutoff(FX_Lofi* lofi, float cutoff_hz);

/**
 * Set saturation amount (0.0-2.0, default 0.0 = no saturation)
 */
void fx_lofi_set_saturation(FX_Lofi* lofi, float amount);

/**
 * Set noise level (0.0-1.0, default 0.0 = no noise)
 */
void fx_lofi_set_noise_level(FX_Lofi* lofi, float level);

/**
 * Set wow/flutter depth (0.0-1.0, default 0.0 = none)
 */
void fx_lofi_set_wow_flutter_depth(FX_Lofi* lofi, float depth);

/**
 * Set wow/flutter rate (0.1-10.0 Hz, default 0.5 Hz)
 */
void fx_lofi_set_wow_flutter_rate(FX_Lofi* lofi, float rate_hz);

// Get parameters
float fx_lofi_get_bit_depth(FX_Lofi* lofi);
float fx_lofi_get_sample_rate_ratio(FX_Lofi* lofi);
float fx_lofi_get_filter_cutoff(FX_Lofi* lofi);
float fx_lofi_get_saturation(FX_Lofi* lofi);
float fx_lofi_get_noise_level(FX_Lofi* lofi);
float fx_lofi_get_wow_flutter_depth(FX_Lofi* lofi);
float fx_lofi_get_wow_flutter_rate(FX_Lofi* lofi);

// ============================================================================
// Processing
// ============================================================================

/**
 * Process audio (stereo interleaved float32)
 * buffer: Input/output buffer (interleaved L/R)
 * frames: Number of stereo frames
 */
void fx_lofi_process_f32(FX_Lofi* lofi, float* buffer, uint32_t frames, uint32_t sample_rate);

#ifdef __cplusplus
}
#endif

#endif // FX_LOFI_H
