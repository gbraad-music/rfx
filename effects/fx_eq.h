/*
 * Regroove 3-Band DJ EQ Effect
 * DJ-style kill EQ with low/mid/high bands
 */

#ifndef FX_EQ_H
#define FX_EQ_H

#include "fx_common.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct FXEqualizer FXEqualizer;

// Lifecycle
FXEqualizer* fx_eq_create(void);
void fx_eq_destroy(FXEqualizer* fx);
void fx_eq_reset(FXEqualizer* fx);

// Processing
void fx_eq_process_f32(FXEqualizer* fx, float* buffer, int frames, int sample_rate);
void fx_eq_process_i16(FXEqualizer* fx, int16_t* buffer, int frames, int sample_rate);
void fx_eq_process_frame(FXEqualizer* fx, float* left, float* right, int sample_rate);

// Parameters (0.0 - 1.0, where 0.5 = neutral)
// 0.0 = KILL, 0.5 = neutral, 1.0 = boost
void fx_eq_set_enabled(FXEqualizer* fx, int enabled);
void fx_eq_set_low(FXEqualizer* fx, float gain);
void fx_eq_set_mid(FXEqualizer* fx, float gain);
void fx_eq_set_high(FXEqualizer* fx, float gain);

int fx_eq_get_enabled(FXEqualizer* fx);
float fx_eq_get_low(FXEqualizer* fx);
float fx_eq_get_mid(FXEqualizer* fx);
float fx_eq_get_high(FXEqualizer* fx);

// ============================================================================
// Generic Parameter Interface (for wrapper use)
// ============================================================================

/**
 * Get total number of parameters
 */
int fx_eq_get_parameter_count(void);

/**
 * Get parameter value by index (normalized 0.0-1.0)
 */
float fx_eq_get_parameter_value(FXEqualizer* fx, int index);

/**
 * Set parameter value by index (normalized 0.0-1.0)
 */
void fx_eq_set_parameter_value(FXEqualizer* fx, int index, float value);

/**
 * Get parameter name
 */
const char* fx_eq_get_parameter_name(int index);

/**
 * Get parameter label/unit
 */
const char* fx_eq_get_parameter_label(int index);

/**
 * Get parameter default value (normalized 0.0-1.0)
 */
float fx_eq_get_parameter_default(int index);

/**
 * Get parameter minimum value (actual units)
 */
float fx_eq_get_parameter_min(int index);

/**
 * Get parameter maximum value (actual units)
 */
float fx_eq_get_parameter_max(int index);

/**
 * Get parameter group/category
 */
int fx_eq_get_parameter_group(int index);

/**
 * Get group name
 */
const char* fx_eq_get_group_name(int group);

/**
 * Check if parameter is integer
 */
int fx_eq_parameter_is_integer(int index);

#ifdef __cplusplus
}
#endif

#endif // FX_EQ_H
