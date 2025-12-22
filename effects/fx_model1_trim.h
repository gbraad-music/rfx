/*
 * FX-Model1 Trim/Drive Effect
 *
 * A standalone effect that emulates the analog overdrive from the
 * input trim control of the Model 1 mixer.
 */

#ifndef FX_MODEL1_TRIM_H
#define FX_MODEL1_TRIM_H

#include "fx_common.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct FXModel1Trim FXModel1Trim;

// Lifecycle
FXModel1Trim* fx_model1_trim_create(void);
void fx_model1_trim_destroy(FXModel1Trim* fx);
void fx_model1_trim_reset(FXModel1Trim* fx);

// Processing
void fx_model1_trim_process_interleaved(FXModel1Trim* fx, float* buffer, int frames, int sample_rate);
void fx_model1_trim_process_f32(FXModel1Trim* fx, float* buffer, int frames, int sample_rate);
void fx_model1_trim_process_frame(FXModel1Trim* fx, float* left, float* right, int sample_rate);

// Parameters (0.0 - 1.0)
void fx_model1_trim_set_enabled(FXModel1Trim* fx, int enabled);
int fx_model1_trim_get_enabled(FXModel1Trim* fx);
void fx_model1_trim_set_drive(FXModel1Trim* fx, float drive);
float fx_model1_trim_get_drive(FXModel1Trim* fx);
float fx_model1_trim_get_peak_level(FXModel1Trim* fx);  // Returns peak output level (0.0+)

// Generic Parameter Interface
int fx_model1_trim_get_parameter_count(void);
float fx_model1_trim_get_parameter_value(FXModel1Trim* fx, int index);
void fx_model1_trim_set_parameter_value(FXModel1Trim* fx, int index, float value);
const char* fx_model1_trim_get_parameter_name(int index);
const char* fx_model1_trim_get_parameter_label(int index);
float fx_model1_trim_get_parameter_default(int index);
float fx_model1_trim_get_parameter_min(int index);
float fx_model1_trim_get_parameter_max(int index);
int fx_model1_trim_get_parameter_group(int index);
const char* fx_model1_trim_get_group_name(int group);
int fx_model1_trim_parameter_is_integer(int index);

#ifdef __cplusplus
}
#endif

#endif // FX_MODEL1_TRIM_H
