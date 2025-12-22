/*
 * Regroove Phaser Effect
 * Cascaded allpass filters with LFO modulation
 */

#ifndef FX_PHASER_H
#define FX_PHASER_H

#include "fx_common.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct FXPhaser FXPhaser;

// Lifecycle
FXPhaser* fx_phaser_create(void);
void fx_phaser_destroy(FXPhaser* fx);
void fx_phaser_reset(FXPhaser* fx);

// Processing
void fx_phaser_process_f32(FXPhaser* fx, float* buffer, int frames, int sample_rate);
void fx_phaser_process_i16(FXPhaser* fx, int16_t* buffer, int frames, int sample_rate);
void fx_phaser_process_frame(FXPhaser* fx, float* left, float* right, int sample_rate);

// Parameters (0.0 - 1.0)
void fx_phaser_set_enabled(FXPhaser* fx, int enabled);
void fx_phaser_set_rate(FXPhaser* fx, float rate);         // LFO rate
void fx_phaser_set_depth(FXPhaser* fx, float depth);       // Modulation depth
void fx_phaser_set_feedback(FXPhaser* fx, float feedback); // Feedback amount

int fx_phaser_get_enabled(FXPhaser* fx);
float fx_phaser_get_rate(FXPhaser* fx);
float fx_phaser_get_depth(FXPhaser* fx);
float fx_phaser_get_feedback(FXPhaser* fx);

// Generic Parameter Interface
int fx_phaser_get_parameter_count(void);
float fx_phaser_get_parameter_value(FXPhaser* fx, int index);
void fx_phaser_set_parameter_value(FXPhaser* fx, int index, float value);
const char* fx_phaser_get_parameter_name(int index);
const char* fx_phaser_get_parameter_label(int index);
float fx_phaser_get_parameter_default(int index);
float fx_phaser_get_parameter_min(int index);
float fx_phaser_get_parameter_max(int index);
int fx_phaser_get_parameter_group(int index);
const char* fx_phaser_get_group_name(int group);
int fx_phaser_parameter_is_integer(int index);

#ifdef __cplusplus
}
#endif

#endif // FX_PHASER_H
