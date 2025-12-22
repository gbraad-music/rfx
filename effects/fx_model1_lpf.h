/*
 * MODEL 1 Contour LPF - Low Pass Filter
 * Copyright (C) 2024
 *
 * Based on PlayDifferently MODEL 1 DJ Mixer
 * - Range: 500 Hz to FLAT (20 kHz)
 * - Low Q (non-resonant)
 * - Designed not to add color to the sound
 */

#ifndef FX_MODEL1_LPF_H
#define FX_MODEL1_LPF_H

#include "fx_common.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct FXModel1LPF FXModel1LPF;

// Create/destroy
FXModel1LPF* fx_model1_lpf_create(void);
void fx_model1_lpf_destroy(FXModel1LPF* fx);
void fx_model1_lpf_reset(FXModel1LPF* fx);

// Process functions
void fx_model1_lpf_process_interleaved(FXModel1LPF* fx, float* buffer, int frames, int sample_rate);
void fx_model1_lpf_process_f32(FXModel1LPF* fx, float* left, float* right, int frames, int sample_rate);
void fx_model1_lpf_process_i16(FXModel1LPF* fx, int16_t* buffer, int frames, int sample_rate);
void fx_model1_lpf_process_frame(FXModel1LPF* fx, float* left, float* right, int sample_rate);

// Setters
void fx_model1_lpf_set_enabled(FXModel1LPF* fx, int enabled);
void fx_model1_lpf_set_cutoff(FXModel1LPF* fx, float cutoff);  // 0.0-1.0 (0.0=500Hz, 1.0=FLAT/20kHz)

// Getters
int fx_model1_lpf_get_enabled(FXModel1LPF* fx);
float fx_model1_lpf_get_cutoff(FXModel1LPF* fx);

// Generic Parameter Interface
int fx_model1_lpf_get_parameter_count(void);
float fx_model1_lpf_get_parameter_value(FXModel1LPF* fx, int index);
void fx_model1_lpf_set_parameter_value(FXModel1LPF* fx, int index, float value);
const char* fx_model1_lpf_get_parameter_name(int index);
const char* fx_model1_lpf_get_parameter_label(int index);
float fx_model1_lpf_get_parameter_default(int index);
float fx_model1_lpf_get_parameter_min(int index);
float fx_model1_lpf_get_parameter_max(int index);
int fx_model1_lpf_get_parameter_group(int index);
const char* fx_model1_lpf_get_group_name(int group);
int fx_model1_lpf_parameter_is_integer(int index);

#ifdef __cplusplus
}
#endif

#endif // FX_MODEL1_LPF_H
