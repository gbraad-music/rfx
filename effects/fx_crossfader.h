/*
 * Regroove DJ Crossfader Effect
 * Blends between two stereo inputs
 */

#ifndef FX_CROSSFADER_H
#define FX_CROSSFADER_H

#include "fx_common.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct FXCrossfader FXCrossfader;

// Lifecycle
FXCrossfader* fx_crossfader_create(void);
void fx_crossfader_destroy(FXCrossfader* fx);
void fx_crossfader_reset(FXCrossfader* fx);

// Processing - takes two stereo inputs, outputs blended result
void fx_crossfader_process_frame(FXCrossfader* fx,
                                  float in_a_left, float in_a_right,
                                  float in_b_left, float in_b_right,
                                  float* out_left, float* out_right,
                                  int sample_rate);

// Parameters (0.0 - 1.0)
void fx_crossfader_set_enabled(FXCrossfader* fx, int enabled);
void fx_crossfader_set_position(FXCrossfader* fx, float position);  // 0.0 = all A, 1.0 = all B
void fx_crossfader_set_curve(FXCrossfader* fx, float curve);        // 0.0 = linear, 1.0 = sharp cut

int fx_crossfader_get_enabled(FXCrossfader* fx);
float fx_crossfader_get_position(FXCrossfader* fx);
float fx_crossfader_get_curve(FXCrossfader* fx);

// ============================================================================
// Generic Parameter Interface (for wrapper use)
// ============================================================================

int fx_crossfader_get_parameter_count(void);
float fx_crossfader_get_parameter_value(FXCrossfader* fx, int index);
void fx_crossfader_set_parameter_value(FXCrossfader* fx, int index, float value);
const char* fx_crossfader_get_parameter_name(int index);
const char* fx_crossfader_get_parameter_label(int index);
float fx_crossfader_get_parameter_default(int index);
float fx_crossfader_get_parameter_min(int index);
float fx_crossfader_get_parameter_max(int index);
int fx_crossfader_get_parameter_group(int index);
const char* fx_crossfader_get_group_name(int group);
int fx_crossfader_parameter_is_integer(int index);

#ifdef __cplusplus
}
#endif

#endif // FX_CROSSFADER_H
