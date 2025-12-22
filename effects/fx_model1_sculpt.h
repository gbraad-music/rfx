/*
 * MODEL 1 Sculpt - Semi-Parametric EQ
 * Copyright (C) 2024
 *
 * Based on PlayDifferently MODEL 1 DJ Mixer
 * - Frequency range: 70 Hz to 7 kHz (~7 octaves)
 * - Gain: -20 dB (cut) to +8 dB (boost), asymmetric
 * - Wide Q (fixed, non-adjustable)
 */

#ifndef FX_MODEL1_SCULPT_H
#define FX_MODEL1_SCULPT_H

#include "fx_common.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct FXModel1Sculpt FXModel1Sculpt;

// Create/destroy
FXModel1Sculpt* fx_model1_sculpt_create(void);
void fx_model1_sculpt_destroy(FXModel1Sculpt* fx);
void fx_model1_sculpt_reset(FXModel1Sculpt* fx);

// Process functions
void fx_model1_sculpt_process_interleaved(FXModel1Sculpt* fx, float* buffer, int frames, int sample_rate);
void fx_model1_sculpt_process_f32(FXModel1Sculpt* fx, float* left, float* right, int frames, int sample_rate);
void fx_model1_sculpt_process_i16(FXModel1Sculpt* fx, int16_t* buffer, int frames, int sample_rate);
void fx_model1_sculpt_process_frame(FXModel1Sculpt* fx, float* left, float* right, int sample_rate);

// Setters
void fx_model1_sculpt_set_enabled(FXModel1Sculpt* fx, int enabled);
void fx_model1_sculpt_set_frequency(FXModel1Sculpt* fx, float freq);  // 0.0-1.0 (70Hz to 7kHz)
void fx_model1_sculpt_set_gain(FXModel1Sculpt* fx, float gain);       // 0.0-1.0 (0.0=-20dB, 0.5=0dB, 1.0=+8dB)

// Getters
int fx_model1_sculpt_get_enabled(FXModel1Sculpt* fx);
float fx_model1_sculpt_get_frequency(FXModel1Sculpt* fx);
float fx_model1_sculpt_get_gain(FXModel1Sculpt* fx);

// Generic Parameter Interface
int fx_model1_sculpt_get_parameter_count(void);
float fx_model1_sculpt_get_parameter_value(FXModel1Sculpt* fx, int index);
void fx_model1_sculpt_set_parameter_value(FXModel1Sculpt* fx, int index, float value);
const char* fx_model1_sculpt_get_parameter_name(int index);
const char* fx_model1_sculpt_get_parameter_label(int index);
float fx_model1_sculpt_get_parameter_default(int index);
float fx_model1_sculpt_get_parameter_min(int index);
float fx_model1_sculpt_get_parameter_max(int index);
int fx_model1_sculpt_get_parameter_group(int index);
const char* fx_model1_sculpt_get_group_name(int group);
int fx_model1_sculpt_parameter_is_integer(int index);

#ifdef __cplusplus
}
#endif

#endif // FX_MODEL1_SCULPT_H
