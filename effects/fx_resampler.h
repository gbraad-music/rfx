/*
 * Regroove Resampler Effect
 * High-quality sample rate conversion and interpolation
 *
 * Based on OpenMPT resampling algorithms (BSD license)
 * Original authors: OpenMPT Devs, Olivier Lapicque
 *
 * Supported interpolation modes:
 * - Nearest (zero-order hold)
 * - Linear (2-point)
 * - Cubic (4-point windowed sinc)
 */

#ifndef FX_RESAMPLER_H
#define FX_RESAMPLER_H

#include "fx_common.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct FXResampler FXResampler;

typedef enum {
    RESAMPLER_NEAREST = 0,    // Zero-order hold (blocky, low CPU)
    RESAMPLER_LINEAR,         // Linear interpolation (good for slight pitch changes)
    RESAMPLER_CUBIC,          // Cubic spline (high quality, low CPU)
    RESAMPLER_NUM_MODES
} ResamplerMode;

// Lifecycle
FXResampler* fx_resampler_create(void);
void fx_resampler_destroy(FXResampler* fx);
void fx_resampler_reset(FXResampler* fx);

// Processing
void fx_resampler_process_f32(FXResampler* fx, const float* input, float* output,
                              int input_frames, int output_frames, int sample_rate);
void fx_resampler_process_frame(FXResampler* fx, const float* input, float* output,
                                double position, int channels);

// Parameters
void fx_resampler_set_enabled(FXResampler* fx, int enabled);
void fx_resampler_set_mode(FXResampler* fx, ResamplerMode mode);
void fx_resampler_set_rate(FXResampler* fx, float rate);  // Playback rate (0.5 = half speed, 2.0 = double speed)

int fx_resampler_get_enabled(FXResampler* fx);
ResamplerMode fx_resampler_get_mode(FXResampler* fx);
float fx_resampler_get_rate(FXResampler* fx);

// Utility
int fx_resampler_get_required_input_frames(int output_frames, float rate);
const char* fx_resampler_get_mode_name(ResamplerMode mode);

// ============================================================================
// Generic Parameter Interface (for wrapper use)
// ============================================================================

int fx_resampler_get_parameter_count(void);
float fx_resampler_get_parameter_value(FXResampler* fx, int index);
void fx_resampler_set_parameter_value(FXResampler* fx, int index, float value);
const char* fx_resampler_get_parameter_name(int index);
const char* fx_resampler_get_parameter_label(int index);
float fx_resampler_get_parameter_default(int index);
float fx_resampler_get_parameter_min(int index);
float fx_resampler_get_parameter_max(int index);
int fx_resampler_get_parameter_group(int index);
const char* fx_resampler_get_group_name(int group);
int fx_resampler_parameter_is_integer(int index);

#ifdef __cplusplus
}
#endif

#endif // FX_RESAMPLER_H
