/*
 * Regroove Distortion Effect
 * Analog-style saturation with drive and mix controls
 */

#ifndef FX_DISTORTION_H
#define FX_DISTORTION_H

#include "fx_common.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct FXDistortion FXDistortion;

// Lifecycle
FXDistortion* fx_distortion_create(void);
void fx_distortion_destroy(FXDistortion* fx);
void fx_distortion_reset(FXDistortion* fx);

// Processing
void fx_distortion_process_f32(FXDistortion* fx, float* buffer, int frames, int sample_rate);
void fx_distortion_process_i16(FXDistortion* fx, int16_t* buffer, int frames, int sample_rate);

// Process single stereo frame (for optimized embedded use)
void fx_distortion_process_frame(FXDistortion* fx, float* left, float* right, int sample_rate);

// Parameters (0.0 - 1.0)
void fx_distortion_set_enabled(FXDistortion* fx, int enabled);
void fx_distortion_set_drive(FXDistortion* fx, float drive);
void fx_distortion_set_mix(FXDistortion* fx, float mix);

int fx_distortion_get_enabled(FXDistortion* fx);
float fx_distortion_get_drive(FXDistortion* fx);
float fx_distortion_get_mix(FXDistortion* fx);

// ============================================================================
// Generic Parameter Interface (for wrapper use)
// ============================================================================

/**
 * Get total number of parameters
 */
int fx_distortion_get_parameter_count(void);

/**
 * Get parameter value by index (normalized 0.0-1.0)
 */
float fx_distortion_get_parameter_value(FXDistortion* fx, int index);

/**
 * Set parameter value by index (normalized 0.0-1.0)
 */
void fx_distortion_set_parameter_value(FXDistortion* fx, int index, float value);

/**
 * Get parameter name
 */
const char* fx_distortion_get_parameter_name(int index);

/**
 * Get parameter label/unit
 */
const char* fx_distortion_get_parameter_label(int index);

/**
 * Get parameter default value (normalized 0.0-1.0)
 */
float fx_distortion_get_parameter_default(int index);

/**
 * Get parameter minimum value (actual units)
 */
float fx_distortion_get_parameter_min(int index);

/**
 * Get parameter maximum value (actual units)
 */
float fx_distortion_get_parameter_max(int index);

/**
 * Get parameter group/category
 */
int fx_distortion_get_parameter_group(int index);

/**
 * Get group name
 */
const char* fx_distortion_get_group_name(int group);

/**
 * Check if parameter is integer
 */
int fx_distortion_parameter_is_integer(int index);

#ifdef __cplusplus
}
#endif

#endif // FX_DISTORTION_H
