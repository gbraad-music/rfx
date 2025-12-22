/*
 * MODEL 1 Contour HPF - High Pass Filter
 * Copyright (C) 2024
 *
 * Based on PlayDifferently MODEL 1 DJ Mixer
 * - Range: FLAT (20 Hz) to 1 kHz
 * - Low Q (non-resonant)
 * - Designed not to add color to the sound
 */

#ifndef FX_MODEL1_HPF_H
#define FX_MODEL1_HPF_H

#include "fx_common.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct FXModel1HPF FXModel1HPF;

// Create/destroy
FXModel1HPF* fx_model1_hpf_create(void);
void fx_model1_hpf_destroy(FXModel1HPF* fx);
void fx_model1_hpf_reset(FXModel1HPF* fx);

// Process functions
void fx_model1_hpf_process_interleaved(FXModel1HPF* fx, float* buffer, int frames, int sample_rate);
void fx_model1_hpf_process_f32(FXModel1HPF* fx, float* left, float* right, int frames, int sample_rate);
void fx_model1_hpf_process_i16(FXModel1HPF* fx, int16_t* buffer, int frames, int sample_rate);
void fx_model1_hpf_process_frame(FXModel1HPF* fx, float* left, float* right, int sample_rate);

// Setters
void fx_model1_hpf_set_enabled(FXModel1HPF* fx, int enabled);
void fx_model1_hpf_set_cutoff(FXModel1HPF* fx, float cutoff);  // 0.0-1.0 (0.0=FLAT/20Hz, 1.0=1kHz)

// Getters
int fx_model1_hpf_get_enabled(FXModel1HPF* fx);
float fx_model1_hpf_get_cutoff(FXModel1HPF* fx);

// Generic Parameter Interface
int fx_model1_hpf_get_parameter_count(void);
float fx_model1_hpf_get_parameter_value(FXModel1HPF* fx, int index);
void fx_model1_hpf_set_parameter_value(FXModel1HPF* fx, int index, float value);
const char* fx_model1_hpf_get_parameter_name(int index);
const char* fx_model1_hpf_get_parameter_label(int index);
float fx_model1_hpf_get_parameter_default(int index);
float fx_model1_hpf_get_parameter_min(int index);
float fx_model1_hpf_get_parameter_max(int index);
int fx_model1_hpf_get_parameter_group(int index);
const char* fx_model1_hpf_get_group_name(int group);
int fx_model1_hpf_parameter_is_integer(int index);

#ifdef __cplusplus
}
#endif

#endif // FX_MODEL1_HPF_H
