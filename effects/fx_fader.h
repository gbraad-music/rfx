/*
 * Regroove DJ Fader Effect
 * Simple volume fader with smooth transitions
 */

#ifndef FX_FADER_H
#define FX_FADER_H

#include "fx_common.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct FXFader FXFader;

// Lifecycle
FXFader* fx_fader_create(void);
void fx_fader_destroy(FXFader* fx);
void fx_fader_reset(FXFader* fx);

// Processing
void fx_fader_process_f32(FXFader* fx, float* buffer, int frames, int sample_rate);
void fx_fader_process_i16(FXFader* fx, int16_t* buffer, int frames, int sample_rate);

// Process single stereo frame (for optimized embedded use)
void fx_fader_process_frame(FXFader* fx, float* left, float* right, int sample_rate);

// Parameters (0.0 - 1.0)
void fx_fader_set_enabled(FXFader* fx, int enabled);
void fx_fader_set_level(FXFader* fx, float level);  // 0.0 = -inf dB, 1.0 = 0 dB

int fx_fader_get_enabled(FXFader* fx);
float fx_fader_get_level(FXFader* fx);

// ============================================================================
// Generic Parameter Interface (for wrapper use)
// ============================================================================

int fx_fader_get_parameter_count(void);
float fx_fader_get_parameter_value(FXFader* fx, int index);
void fx_fader_set_parameter_value(FXFader* fx, int index, float value);
const char* fx_fader_get_parameter_name(int index);
const char* fx_fader_get_parameter_label(int index);
float fx_fader_get_parameter_default(int index);
float fx_fader_get_parameter_min(int index);
float fx_fader_get_parameter_max(int index);
int fx_fader_get_parameter_group(int index);
const char* fx_fader_get_group_name(int group);
int fx_fader_parameter_is_integer(int index);

#ifdef __cplusplus
}
#endif

#endif // FX_FADER_H
