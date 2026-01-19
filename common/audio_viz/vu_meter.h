/**
 * VU Meter - Core Logic (Framework-Agnostic)
 *
 * Provides peak/RMS calculation, ballistics, decay, and dB conversion
 * NO UI DEPENDENCIES - pure DSP/logic
 *
 * Usage:
 *   1. Create: VUMeter vu; vu_meter_init(&vu, sample_rate);
 *   2. Process: vu_meter_process(&vu, left_sample, right_sample);
 *   3. Read: vu.peak_left_db, vu.rms_left_db, vu.peak_hold_left_db
 *   4. Render: Use framework-specific wrapper (ImGui, Rack, Canvas)
 *
 * Copyright (C) 2025
 * SPDX-License-Identifier: ISC
 */

#ifndef VU_METER_H
#define VU_METER_H

#include <stdint.h>
#include <math.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// Configuration
// ============================================================================

typedef enum {
    VU_MODE_PEAK,           // Digital peak meter (fast attack, slow release)
    VU_MODE_RMS,            // RMS (average power)
    VU_MODE_VU_CLASSIC,     // Classic VU ballistics (300ms integration)
    VU_MODE_PPM            // Peak Programme Meter (BBC/EBU)
} VUMeterMode;

// ============================================================================
// VU Meter State
// ============================================================================

typedef struct {
    // Current levels (linear 0.0-1.0+)
    float peak_left;
    float peak_right;
    float rms_left;
    float rms_right;

    // Current levels in dB
    float peak_left_db;
    float peak_right_db;
    float rms_left_db;
    float rms_right_db;

    // Peak hold
    float peak_hold_left;
    float peak_hold_right;
    float peak_hold_left_db;
    float peak_hold_right_db;
    uint32_t peak_hold_timer_left;
    uint32_t peak_hold_timer_right;

    // RMS calculation (sliding window)
    float rms_sum_left;
    float rms_sum_right;
    uint32_t rms_count;

    // Configuration
    VUMeterMode mode;
    float decay_rate;           // dB per second
    float peak_hold_time;       // seconds
    uint32_t sample_rate;
    uint32_t rms_window_samples; // RMS integration window

    // Internal
    float decay_coeff;          // Calculated from decay_rate
    uint32_t peak_hold_samples; // Calculated from peak_hold_time
} VUMeter;

// ============================================================================
// Conversion Utilities
// ============================================================================

/**
 * Convert linear (0.0-1.0+) to dB
 * Returns -96.0 dB for silence
 */
static inline float vu_linear_to_db(float linear) {
    if (linear <= 0.0f) return -96.0f;
    return 20.0f * log10f(linear);
}

/**
 * Convert dB to linear (0.0-1.0+)
 */
static inline float vu_db_to_linear(float db) {
    return powf(10.0f, db / 20.0f);
}

/**
 * Clamp dB value to reasonable range
 */
static inline float vu_clamp_db(float db) {
    if (db < -96.0f) return -96.0f;
    if (db > 12.0f) return 12.0f;
    return db;
}

// ============================================================================
// Initialization
// ============================================================================

/**
 * Initialize VU meter
 *
 * @param vu Pointer to VU meter structure
 * @param sample_rate Sample rate in Hz
 * @param mode Meter mode (PEAK, RMS, VU_CLASSIC, PPM)
 */
static inline void vu_meter_init(VUMeter* vu, uint32_t sample_rate, VUMeterMode mode) {
    memset(vu, 0, sizeof(VUMeter));

    vu->sample_rate = sample_rate;
    vu->mode = mode;

    // Set defaults based on mode
    switch (mode) {
        case VU_MODE_PEAK:
            vu->decay_rate = 24.0f;          // Fast decay: 24 dB/sec
            vu->peak_hold_time = 2.0f;       // Hold for 2 seconds
            vu->rms_window_samples = sample_rate / 20; // 50ms window
            break;

        case VU_MODE_RMS:
            vu->decay_rate = 12.0f;          // Slower decay for RMS
            vu->peak_hold_time = 1.5f;
            vu->rms_window_samples = sample_rate / 10; // 100ms window
            break;

        case VU_MODE_VU_CLASSIC:
            vu->decay_rate = 10.0f;          // Classic VU: slow decay
            vu->peak_hold_time = 0.0f;       // No peak hold in classic VU
            vu->rms_window_samples = (uint32_t)(sample_rate * 0.3f); // 300ms integration
            break;

        case VU_MODE_PPM:
            vu->decay_rate = 20.0f;          // PPM: controlled decay
            vu->peak_hold_time = 0.0f;
            vu->rms_window_samples = sample_rate / 50; // 20ms window
            break;
    }

    // Calculate derived values
    vu->decay_coeff = 1.0f - (vu->decay_rate / (20.0f * sample_rate)); // Convert dB/sec to per-sample coefficient
    vu->peak_hold_samples = (uint32_t)(vu->peak_hold_time * sample_rate);

    // Initialize to silence
    vu->peak_left_db = -96.0f;
    vu->peak_right_db = -96.0f;
    vu->rms_left_db = -96.0f;
    vu->rms_right_db = -96.0f;
    vu->peak_hold_left_db = -96.0f;
    vu->peak_hold_right_db = -96.0f;
}

/**
 * Reset VU meter to silence
 */
static inline void vu_meter_reset(VUMeter* vu) {
    vu->peak_left = 0.0f;
    vu->peak_right = 0.0f;
    vu->rms_left = 0.0f;
    vu->rms_right = 0.0f;
    vu->peak_hold_left = 0.0f;
    vu->peak_hold_right = 0.0f;
    vu->rms_sum_left = 0.0f;
    vu->rms_sum_right = 0.0f;
    vu->rms_count = 0;
    vu->peak_hold_timer_left = 0;
    vu->peak_hold_timer_right = 0;

    vu->peak_left_db = -96.0f;
    vu->peak_right_db = -96.0f;
    vu->rms_left_db = -96.0f;
    vu->rms_right_db = -96.0f;
    vu->peak_hold_left_db = -96.0f;
    vu->peak_hold_right_db = -96.0f;
}

// ============================================================================
// Processing
// ============================================================================

/**
 * Process one stereo sample
 * Call this for every sample pair (left, right)
 *
 * @param vu Pointer to VU meter structure
 * @param left Left channel sample (-1.0 to +1.0)
 * @param right Right channel sample (-1.0 to +1.0)
 */
static inline void vu_meter_process(VUMeter* vu, float left, float right) {
    // Get absolute values
    float abs_left = fabsf(left);
    float abs_right = fabsf(right);

    // === PEAK DETECTION ===
    // Instant attack, controlled decay
    if (abs_left > vu->peak_left) {
        vu->peak_left = abs_left;
        vu->peak_hold_timer_left = vu->peak_hold_samples;
    } else {
        // Apply decay
        vu->peak_left *= vu->decay_coeff;
        if (vu->peak_left < 0.00001f) vu->peak_left = 0.0f;
    }

    if (abs_right > vu->peak_right) {
        vu->peak_right = abs_right;
        vu->peak_hold_timer_right = vu->peak_hold_samples;
    } else {
        vu->peak_right *= vu->decay_coeff;
        if (vu->peak_right < 0.00001f) vu->peak_right = 0.0f;
    }

    // === PEAK HOLD ===
    if (vu->peak_hold_time > 0.0f) {
        if (vu->peak_left > vu->peak_hold_left) {
            vu->peak_hold_left = vu->peak_left;
            vu->peak_hold_timer_left = vu->peak_hold_samples;
        } else if (vu->peak_hold_timer_left > 0) {
            vu->peak_hold_timer_left--;
        } else {
            // Decay peak hold slowly
            vu->peak_hold_left *= 0.9999f;
            if (vu->peak_hold_left < 0.00001f) vu->peak_hold_left = 0.0f;
        }

        if (vu->peak_right > vu->peak_hold_right) {
            vu->peak_hold_right = vu->peak_right;
            vu->peak_hold_timer_right = vu->peak_hold_samples;
        } else if (vu->peak_hold_timer_right > 0) {
            vu->peak_hold_timer_right--;
        } else {
            vu->peak_hold_right *= 0.9999f;
            if (vu->peak_hold_right < 0.00001f) vu->peak_hold_right = 0.0f;
        }
    }

    // === RMS CALCULATION ===
    // Sliding window average
    vu->rms_sum_left += left * left;
    vu->rms_sum_right += right * right;
    vu->rms_count++;

    if (vu->rms_count >= vu->rms_window_samples) {
        vu->rms_left = sqrtf(vu->rms_sum_left / vu->rms_count);
        vu->rms_right = sqrtf(vu->rms_sum_right / vu->rms_count);

        // Reset accumulator
        vu->rms_sum_left = 0.0f;
        vu->rms_sum_right = 0.0f;
        vu->rms_count = 0;
    }

    // === CONVERT TO dB ===
    vu->peak_left_db = vu_clamp_db(vu_linear_to_db(vu->peak_left));
    vu->peak_right_db = vu_clamp_db(vu_linear_to_db(vu->peak_right));
    vu->rms_left_db = vu_clamp_db(vu_linear_to_db(vu->rms_left));
    vu->rms_right_db = vu_clamp_db(vu_linear_to_db(vu->rms_right));
    vu->peak_hold_left_db = vu_clamp_db(vu_linear_to_db(vu->peak_hold_left));
    vu->peak_hold_right_db = vu_clamp_db(vu_linear_to_db(vu->peak_hold_right));
}

/**
 * Process a buffer of stereo samples
 *
 * @param vu Pointer to VU meter structure
 * @param left_buffer Left channel buffer
 * @param right_buffer Right channel buffer
 * @param num_samples Number of samples in each buffer
 */
static inline void vu_meter_process_buffer(VUMeter* vu, const float* left_buffer,
                                           const float* right_buffer, uint32_t num_samples) {
    for (uint32_t i = 0; i < num_samples; i++) {
        vu_meter_process(vu, left_buffer[i], right_buffer[i]);
    }
}

/**
 * Process interleaved stereo buffer (L, R, L, R, ...)
 */
static inline void vu_meter_process_interleaved(VUMeter* vu, const float* buffer, uint32_t frames) {
    for (uint32_t i = 0; i < frames; i++) {
        vu_meter_process(vu, buffer[i * 2], buffer[i * 2 + 1]);
    }
}

// ============================================================================
// Accessors
// ============================================================================

/**
 * Get current peak level in dB for a channel
 * @param vu VU meter
 * @param channel 0 = left, 1 = right
 * @return Peak level in dB (-96.0 to +12.0)
 */
static inline float vu_meter_get_peak_db(const VUMeter* vu, int channel) {
    return (channel == 0) ? vu->peak_left_db : vu->peak_right_db;
}

/**
 * Get current RMS level in dB for a channel
 */
static inline float vu_meter_get_rms_db(const VUMeter* vu, int channel) {
    return (channel == 0) ? vu->rms_left_db : vu->rms_right_db;
}

/**
 * Get peak hold level in dB for a channel
 */
static inline float vu_meter_get_peak_hold_db(const VUMeter* vu, int channel) {
    return (channel == 0) ? vu->peak_hold_left_db : vu->peak_hold_right_db;
}

/**
 * Get normalized meter position (0.0 to 1.0) for rendering
 * Maps -96dB to 0dB range to 0.0-1.0
 */
static inline float vu_meter_get_normalized(float db) {
    // Map -96dB (silence) to 0.0, 0dB (unity) to 1.0
    return (db + 96.0f) / 96.0f;
}

#ifdef __cplusplus
}
#endif

#endif // VU_METER_H
