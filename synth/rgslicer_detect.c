/**
 * RGSlicer - Transient Detection & Auto-Slicing
 */

#include "rgslicer.h"
#include <stdlib.h>
#include <math.h>
#include <stdio.h>

// External access to RGSlicer internals for detection
extern int rgslicer_add_slice(RGSlicer* slicer, uint32_t offset);

// ============================================================================
// Transient Detection Algorithm
// ============================================================================

static float calculate_rms(int16_t* data, uint32_t start, uint32_t window_size) {
    double sum = 0.0;
    for (uint32_t i = 0; i < window_size; i++) {
        float sample = (float)data[start + i] / 32768.0f;
        sum += sample * sample;
    }
    return sqrtf(sum / window_size);
}

/**
 * Detect transients using energy-based onset detection
 */
static uint8_t detect_transients(RGSlicer* slicer, uint32_t* slice_points, uint8_t max_slices, float sensitivity) {
    if (!slicer || !slicer->sample_data) return 0;

    uint32_t sample_length = slicer->sample_length;
    int16_t* data = slicer->sample_data;
    uint8_t found = 0;

    // Parameters
    uint32_t window_size = slicer->sample_rate / 100;  // 10ms window
    float threshold = 0.1f + (sensitivity * 0.4f);     // 0.1 to 0.5
    uint32_t min_gap = slicer->sample_rate / 20;       // 50ms minimum gap

    // Calculate RMS energy across windows
    uint32_t num_windows = (sample_length - window_size) / (window_size / 2);
    float prev_rms = 0.0f;

    for (uint32_t w = 0; w < num_windows && found < max_slices; w++) {
        uint32_t pos = w * (window_size / 2);
        if (pos + window_size >= sample_length) break;

        float rms = calculate_rms(data, pos, window_size);

        // Detect sudden increase in energy (onset)
        if (rms > threshold && rms > prev_rms * 1.5f) {
            // Check minimum gap from previous slice
            if (found == 0 || (pos - slice_points[found - 1]) > min_gap) {
                // Find exact peak within window
                uint32_t peak_pos = pos;
                float peak_val = 0.0f;

                for (uint32_t i = 0; i < window_size; i++) {
                    float abs_sample = fabsf((float)data[pos + i] / 32768.0f);
                    if (abs_sample > peak_val) {
                        peak_val = abs_sample;
                        peak_pos = pos + i;
                    }
                }

                slice_points[found++] = peak_pos;
                printf("[RGSlicer] Transient detected at sample %u (RMS: %.3f)\n", peak_pos, rms);
            }
        }

        prev_rms = rms;
    }

    return found;
}

// ============================================================================
// Zero-Crossing Detection
// ============================================================================

static uint8_t detect_zero_crossings(RGSlicer* slicer, uint32_t* slice_points, uint8_t max_slices) {
    if (!slicer || !slicer->sample_data) return 0;

    uint32_t sample_length = slicer->sample_length;
    int16_t* data = slicer->sample_data;
    uint8_t found = 0;

    uint32_t min_gap = slicer->sample_rate / 10;  // 100ms minimum
    int16_t prev_sample = data[0];

    for (uint32_t i = 1; i < sample_length && found < max_slices; i++) {
        int16_t curr_sample = data[i];

        // Detect zero crossing (sign change)
        if ((prev_sample < 0 && curr_sample >= 0) || (prev_sample >= 0 && curr_sample < 0)) {
            // Check minimum gap
            if (found == 0 || (i - slice_points[found - 1]) > min_gap) {
                slice_points[found++] = i;
            }
        }

        prev_sample = curr_sample;
    }

    return found;
}

// ============================================================================
// Fixed Grid Slicing
// ============================================================================

static uint8_t slice_fixed_grid(RGSlicer* slicer, uint32_t* slice_points, uint8_t num_slices) {
    if (!slicer || !slicer->sample_data || num_slices == 0) return 0;

    uint32_t slice_length = slicer->sample_length / num_slices;

    for (uint8_t i = 0; i < num_slices; i++) {
        slice_points[i] = i * slice_length;
    }

    printf("[RGSlicer] Created %d equal slices, length: %u samples\n", num_slices, slice_length);

    return num_slices;
}

// ============================================================================
// BPM-Synced Slicing
// ============================================================================

static uint8_t slice_bpm_sync(RGSlicer* slicer, uint32_t* slice_points, uint8_t num_slices, float bpm) {
    if (!slicer || !slicer->sample_data || bpm <= 0.0f) return 0;

    // Calculate samples per beat
    float samples_per_beat = (60.0f / bpm) * slicer->sample_rate;

    // Calculate slice duration based on desired divisions
    // Assume we want to slice into 16th notes
    float samples_per_slice = samples_per_beat / 4.0f;  // 16th note = 1/4 beat

    uint8_t found = 0;
    for (uint8_t i = 0; i < num_slices; i++) {
        uint32_t pos = (uint32_t)(i * samples_per_slice);
        if (pos >= slicer->sample_length) break;

        slice_points[found++] = pos;
    }

    printf("[RGSlicer] BPM-synced slicing: %.1f BPM, %u samples/beat\n", bpm, (uint32_t)samples_per_beat);

    return found;
}

// ============================================================================
// Public Auto-Slice Function
// ============================================================================

uint8_t rgslicer_auto_slice(RGSlicer* slicer, SliceMode mode, uint8_t num_slices, float sensitivity) {
    if (!slicer || !slicer->sample_loaded) {
        printf("[RGSlicer] Cannot auto-slice: no sample loaded\n");
        return 0;
    }

    if (num_slices == 0 || num_slices > RGSLICER_MAX_SLICES) {
        printf("[RGSlicer] Invalid num_slices: %d (max %d)\n", num_slices, RGSLICER_MAX_SLICES);
        return 0;
    }

    // Clear existing slices
    rgslicer_clear_slices(slicer);

    uint32_t slice_points[RGSLICER_MAX_SLICES];
    uint8_t found = 0;

    // Run detection algorithm
    switch (mode) {
        case SLICE_TRANSIENT:
            printf("[RGSlicer] Auto-slicing: Transient detection (sensitivity: %.2f)\n", sensitivity);
            found = detect_transients(slicer, slice_points, num_slices, sensitivity);
            break;

        case SLICE_ZERO_CROSSING:
            printf("[RGSlicer] Auto-slicing: Zero-crossing detection\n");
            found = detect_zero_crossings(slicer, slice_points, num_slices);
            break;

        case SLICE_FIXED_GRID:
            printf("[RGSlicer] Auto-slicing: Fixed grid (%d slices)\n", num_slices);
            found = slice_fixed_grid(slicer, slice_points, num_slices);
            break;

        case SLICE_BPM_SYNC:
            printf("[RGSlicer] Auto-slicing: BPM sync (%.1f BPM)\n", slicer->bpm);
            found = slice_bpm_sync(slicer, slice_points, num_slices, slicer->bpm);
            break;

        default:
            printf("[RGSlicer] Unknown slice mode: %d\n", mode);
            return 0;
    }

    // Create slices at detected points
    for (uint8_t i = 0; i < found; i++) {
        rgslicer_add_slice(slicer, slice_points[i]);
    }

    printf("[RGSlicer] Created %d slices\n", found);

    return found;
}
