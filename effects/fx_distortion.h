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

#ifdef __cplusplus
}
#endif

#endif // FX_DISTORTION_H
