/**
 * Waveform Display - Core Logic (Framework-Agnostic)
 *
 * Provides waveform buffer management, zoom/pan, and coordinate conversion
 * NO UI DEPENDENCIES - pure logic
 *
 * Usage:
 *   1. Create: WaveformDisplay wf; waveform_init(&wf, buffer_size);
 *   2. Write: waveform_write_samples(&wf, samples, count);
 *   3. Read: waveform_get_sample(&wf, index) or waveform_get_envelope(&wf, ...)
 *   4. Render: Use framework-specific wrapper (ImGui, Rack, Canvas)
 *
 * Features:
 *   - Ring buffer for real-time streaming
 *   - Static buffer for loaded samples
 *   - Zoom (1x-100x) and pan support
 *   - Min/max envelope for efficient rendering
 *   - Coordinate conversion helpers
 *
 * Copyright (C) 2025
 * SPDX-License-Identifier: ISC
 */

#ifndef WAVEFORM_H
#define WAVEFORM_H

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// Configuration
// ============================================================================

#define WAVEFORM_MAX_BUFFER_SIZE (48000 * 10)  // 10 seconds at 48kHz

typedef enum {
    WAVEFORM_MODE_STREAMING,    // Ring buffer, continuous updates
    WAVEFORM_MODE_STATIC        // Fixed buffer (loaded sample)
} WaveformMode;

typedef enum {
    WAVEFORM_CHANNEL_MONO,      // Single channel
    WAVEFORM_CHANNEL_STEREO,    // Left + Right separate
    WAVEFORM_CHANNEL_MID_SIDE   // Mid/Side encoding
} WaveformChannelMode;

// ============================================================================
// Waveform Display State
// ============================================================================

typedef struct {
    // Buffer storage
    float* buffer_left;          // Left/mono channel samples
    float* buffer_right;         // Right channel samples (NULL for mono)
    uint32_t buffer_size;        // Maximum buffer capacity
    uint32_t write_position;     // Current write head (for streaming)
    uint32_t sample_count;       // Number of valid samples in buffer

    // Configuration
    WaveformMode mode;
    WaveformChannelMode channel_mode;
    uint32_t sample_rate;

    // Zoom and pan state
    float zoom_level;            // 1.0 = 100%, up to 100.0
    float pan_offset;            // Normalized 0.0-1.0 (position of left edge)

    // Vertical scaling
    float amplitude_scale;       // 1.0 = normal, >1.0 = zoom in, <1.0 = zoom out

    // Flags
    uint8_t owns_buffer;         // True if we allocated the buffer
} WaveformDisplay;

// ============================================================================
// Initialization
// ============================================================================

/**
 * Initialize waveform display with ring buffer
 *
 * @param wf Pointer to WaveformDisplay structure
 * @param buffer_size Size of ring buffer in samples
 * @param channel_mode Mono or stereo
 * @param sample_rate Sample rate in Hz
 * @return 0 on success, -1 on failure
 */
static inline int waveform_init(WaveformDisplay* wf, uint32_t buffer_size,
                                WaveformChannelMode channel_mode, uint32_t sample_rate) {
    if (!wf || buffer_size == 0 || buffer_size > WAVEFORM_MAX_BUFFER_SIZE) {
        return -1;
    }

    memset(wf, 0, sizeof(WaveformDisplay));

    // Allocate buffers
    wf->buffer_left = (float*)calloc(buffer_size, sizeof(float));
    if (!wf->buffer_left) return -1;

    if (channel_mode == WAVEFORM_CHANNEL_STEREO || channel_mode == WAVEFORM_CHANNEL_MID_SIDE) {
        wf->buffer_right = (float*)calloc(buffer_size, sizeof(float));
        if (!wf->buffer_right) {
            free(wf->buffer_left);
            return -1;
        }
    }

    wf->buffer_size = buffer_size;
    wf->mode = WAVEFORM_MODE_STREAMING;
    wf->channel_mode = channel_mode;
    wf->sample_rate = sample_rate;
    wf->zoom_level = 1.0f;
    wf->pan_offset = 0.0f;
    wf->amplitude_scale = 1.0f;
    wf->owns_buffer = 1;
    wf->write_position = 0;
    wf->sample_count = 0;

    return 0;
}

/**
 * Initialize waveform display with external static buffer
 * (For displaying loaded samples - no copying)
 *
 * @param wf Pointer to WaveformDisplay structure
 * @param left_buffer External left/mono buffer
 * @param right_buffer External right buffer (NULL for mono)
 * @param num_samples Number of samples in buffer
 * @param sample_rate Sample rate in Hz
 */
static inline void waveform_init_static(WaveformDisplay* wf,
                                       float* left_buffer, float* right_buffer,
                                       uint32_t num_samples, uint32_t sample_rate) {
    memset(wf, 0, sizeof(WaveformDisplay));

    wf->buffer_left = left_buffer;
    wf->buffer_right = right_buffer;
    wf->buffer_size = num_samples;
    wf->sample_count = num_samples;
    wf->mode = WAVEFORM_MODE_STATIC;
    wf->channel_mode = right_buffer ? WAVEFORM_CHANNEL_STEREO : WAVEFORM_CHANNEL_MONO;
    wf->sample_rate = sample_rate;
    wf->zoom_level = 1.0f;
    wf->pan_offset = 0.0f;
    wf->amplitude_scale = 1.0f;
    wf->owns_buffer = 0;  // Don't free external buffer
}

/**
 * Cleanup waveform display
 */
static inline void waveform_destroy(WaveformDisplay* wf) {
    if (!wf) return;

    if (wf->owns_buffer) {
        if (wf->buffer_left) free(wf->buffer_left);
        if (wf->buffer_right) free(wf->buffer_right);
    }

    memset(wf, 0, sizeof(WaveformDisplay));
}

/**
 * Clear buffer contents
 */
static inline void waveform_clear(WaveformDisplay* wf) {
    if (!wf) return;

    if (wf->buffer_left) {
        memset(wf->buffer_left, 0, wf->buffer_size * sizeof(float));
    }
    if (wf->buffer_right) {
        memset(wf->buffer_right, 0, wf->buffer_size * sizeof(float));
    }

    wf->write_position = 0;
    wf->sample_count = 0;
}

// ============================================================================
// Writing Samples (Streaming Mode)
// ============================================================================

/**
 * Write mono samples to ring buffer
 */
static inline void waveform_write_mono(WaveformDisplay* wf, const float* samples, uint32_t count) {
    if (!wf || !samples || !wf->buffer_left || wf->mode != WAVEFORM_MODE_STREAMING) return;

    for (uint32_t i = 0; i < count; i++) {
        wf->buffer_left[wf->write_position] = samples[i];

        wf->write_position = (wf->write_position + 1) % wf->buffer_size;

        if (wf->sample_count < wf->buffer_size) {
            wf->sample_count++;
        }
    }
}

/**
 * Write stereo samples to ring buffer (interleaved L, R, L, R...)
 */
static inline void waveform_write_stereo(WaveformDisplay* wf, const float* samples, uint32_t frames) {
    if (!wf || !samples || !wf->buffer_left || !wf->buffer_right ||
        wf->mode != WAVEFORM_MODE_STREAMING) return;

    for (uint32_t i = 0; i < frames; i++) {
        wf->buffer_left[wf->write_position] = samples[i * 2];
        wf->buffer_right[wf->write_position] = samples[i * 2 + 1];

        wf->write_position = (wf->write_position + 1) % wf->buffer_size;

        if (wf->sample_count < wf->buffer_size) {
            wf->sample_count++;
        }
    }
}

/**
 * Write separate L/R buffers
 */
static inline void waveform_write_separate(WaveformDisplay* wf,
                                          const float* left, const float* right,
                                          uint32_t frames) {
    if (!wf || !left || !wf->buffer_left || wf->mode != WAVEFORM_MODE_STREAMING) return;

    for (uint32_t i = 0; i < frames; i++) {
        wf->buffer_left[wf->write_position] = left[i];

        if (wf->buffer_right && right) {
            wf->buffer_right[wf->write_position] = right[i];
        }

        wf->write_position = (wf->write_position + 1) % wf->buffer_size;

        if (wf->sample_count < wf->buffer_size) {
            wf->sample_count++;
        }
    }
}

// ============================================================================
// Reading Samples
// ============================================================================

/**
 * Get sample at absolute index (handles ring buffer wrapping)
 *
 * @param wf Waveform display
 * @param index Absolute sample index (0 = oldest in ring buffer)
 * @param channel 0 = left/mono, 1 = right
 * @return Sample value
 */
static inline float waveform_get_sample(const WaveformDisplay* wf, uint32_t index, int channel) {
    if (!wf || index >= wf->sample_count) return 0.0f;

    uint32_t actual_index;

    if (wf->mode == WAVEFORM_MODE_STREAMING) {
        // Ring buffer: calculate actual position
        // Oldest sample is at (write_position - sample_count)
        uint32_t oldest = (wf->write_position + wf->buffer_size - wf->sample_count) % wf->buffer_size;
        actual_index = (oldest + index) % wf->buffer_size;
    } else {
        // Static buffer: direct indexing
        actual_index = index;
    }

    if (channel == 0 && wf->buffer_left) {
        return wf->buffer_left[actual_index];
    } else if (channel == 1 && wf->buffer_right) {
        return wf->buffer_right[actual_index];
    }

    return 0.0f;
}

/**
 * Get min/max envelope for a range of samples (for efficient rendering)
 *
 * @param wf Waveform display
 * @param start_index Start sample index
 * @param end_index End sample index (exclusive)
 * @param channel 0 = left/mono, 1 = right
 * @param out_min Output: minimum value in range
 * @param out_max Output: maximum value in range
 */
static inline void waveform_get_envelope(const WaveformDisplay* wf, uint32_t start_index,
                                        uint32_t end_index, int channel,
                                        float* out_min, float* out_max) {
    if (!wf || !out_min || !out_max || start_index >= wf->sample_count) {
        *out_min = 0.0f;
        *out_max = 0.0f;
        return;
    }

    // Clamp end index
    if (end_index > wf->sample_count) {
        end_index = wf->sample_count;
    }

    if (start_index >= end_index) {
        *out_min = 0.0f;
        *out_max = 0.0f;
        return;
    }

    float min_val = waveform_get_sample(wf, start_index, channel);
    float max_val = min_val;

    for (uint32_t i = start_index + 1; i < end_index; i++) {
        float sample = waveform_get_sample(wf, i, channel);
        if (sample < min_val) min_val = sample;
        if (sample > max_val) max_val = sample;
    }

    *out_min = min_val;
    *out_max = max_val;
}

// ============================================================================
// Zoom and Pan
// ============================================================================

/**
 * Set zoom level (1.0 = 100%, 100.0 = max zoom)
 */
static inline void waveform_set_zoom(WaveformDisplay* wf, float zoom) {
    if (!wf) return;

    if (zoom < 1.0f) zoom = 1.0f;
    if (zoom > 100.0f) zoom = 100.0f;

    wf->zoom_level = zoom;

    // Clamp pan offset to valid range with new zoom
    uint32_t visible_samples = (uint32_t)(wf->sample_count / wf->zoom_level);
    if (visible_samples == 0) visible_samples = 1;

    uint32_t max_start = wf->sample_count > visible_samples ?
                         wf->sample_count - visible_samples : 0;

    float max_pan = (float)max_start / (float)wf->sample_count;
    if (wf->pan_offset > max_pan) {
        wf->pan_offset = max_pan;
    }
}

/**
 * Set pan offset (0.0 = beginning, 1.0 = end)
 */
static inline void waveform_set_pan(WaveformDisplay* wf, float pan) {
    if (!wf) return;

    if (pan < 0.0f) pan = 0.0f;
    if (pan > 1.0f) pan = 1.0f;

    wf->pan_offset = pan;
}

/**
 * Set amplitude scale (1.0 = normal, 2.0 = 2x vertical zoom)
 */
static inline void waveform_set_amplitude_scale(WaveformDisplay* wf, float scale) {
    if (!wf) return;

    if (scale < 0.1f) scale = 0.1f;
    if (scale > 10.0f) scale = 10.0f;

    wf->amplitude_scale = scale;
}

// ============================================================================
// Coordinate Conversion
// ============================================================================

/**
 * Get visible sample range for current zoom/pan
 *
 * @param wf Waveform display
 * @param out_start Output: first visible sample index
 * @param out_count Output: number of visible samples
 */
static inline void waveform_get_visible_range(const WaveformDisplay* wf,
                                             uint32_t* out_start, uint32_t* out_count) {
    if (!wf || !out_start || !out_count) return;

    uint32_t visible_samples = (uint32_t)(wf->sample_count / wf->zoom_level);
    if (visible_samples == 0) visible_samples = 1;
    if (visible_samples > wf->sample_count) visible_samples = wf->sample_count;

    uint32_t start_sample = (uint32_t)(wf->pan_offset * wf->sample_count);

    // Ensure we don't go past the end
    if (start_sample + visible_samples > wf->sample_count) {
        start_sample = wf->sample_count - visible_samples;
    }

    *out_start = start_sample;
    *out_count = visible_samples;
}

/**
 * Convert normalized position (0.0-1.0) to sample index
 * Accounts for zoom and pan
 *
 * @param wf Waveform display
 * @param normalized_pos Position (0.0 = left edge, 1.0 = right edge)
 * @return Sample index
 */
static inline uint32_t waveform_normalized_to_sample(const WaveformDisplay* wf, float normalized_pos) {
    if (!wf) return 0;

    if (normalized_pos < 0.0f) normalized_pos = 0.0f;
    if (normalized_pos > 1.0f) normalized_pos = 1.0f;

    uint32_t start_sample, visible_samples;
    waveform_get_visible_range(wf, &start_sample, &visible_samples);

    uint32_t sample_idx = start_sample + (uint32_t)(normalized_pos * visible_samples);

    if (sample_idx >= wf->sample_count) {
        sample_idx = wf->sample_count - 1;
    }

    return sample_idx;
}

/**
 * Convert sample index to normalized position (0.0-1.0)
 * Returns -1.0 if sample is not visible
 *
 * @param wf Waveform display
 * @param sample_idx Sample index
 * @return Normalized position or -1.0 if not visible
 */
static inline float waveform_sample_to_normalized(const WaveformDisplay* wf, uint32_t sample_idx) {
    if (!wf || sample_idx >= wf->sample_count) return -1.0f;

    uint32_t start_sample, visible_samples;
    waveform_get_visible_range(wf, &start_sample, &visible_samples);

    uint32_t end_sample = start_sample + visible_samples;

    // Check if visible
    if (sample_idx < start_sample || sample_idx > end_sample) {
        return -1.0f;
    }

    float normalized = (float)(sample_idx - start_sample) / (float)visible_samples;
    return normalized;
}

/**
 * Get time in seconds for a sample index
 */
static inline float waveform_sample_to_time(const WaveformDisplay* wf, uint32_t sample_idx) {
    if (!wf || wf->sample_rate == 0) return 0.0f;
    return (float)sample_idx / (float)wf->sample_rate;
}

/**
 * Get sample index for a time in seconds
 */
static inline uint32_t waveform_time_to_sample(const WaveformDisplay* wf, float time_seconds) {
    if (!wf) return 0;

    uint32_t sample_idx = (uint32_t)(time_seconds * wf->sample_rate);

    if (sample_idx >= wf->sample_count) {
        sample_idx = wf->sample_count > 0 ? wf->sample_count - 1 : 0;
    }

    return sample_idx;
}

#ifdef __cplusplus
}
#endif

#endif // WAVEFORM_H
