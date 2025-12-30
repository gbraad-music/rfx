#ifndef FX_STEREO_WIDEN_H
#define FX_STEREO_WIDEN_H

#include "fx_common.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct FXStereoWiden FXStereoWiden;

FXStereoWiden* fx_stereo_widen_create(void);
void fx_stereo_widen_destroy(FXStereoWiden* fx);
void fx_stereo_widen_reset(FXStereoWiden* fx);
void fx_stereo_widen_set_enabled(FXStereoWiden* fx, bool enabled);

// Parameters: 0..1
void fx_stereo_widen_set_width(FXStereoWiden* fx, fx_param_t width);   // 0 = mono, 1 = max widen
void fx_stereo_widen_set_mix(FXStereoWiden* fx, fx_param_t mix);       // dry/wet

bool fx_stereo_widen_get_enabled(FXStereoWiden* fx);
float fx_stereo_widen_get_width(FXStereoWiden* fx);
float fx_stereo_widen_get_mix(FXStereoWiden* fx);

void fx_stereo_widen_process_f32(FXStereoWiden* fx, const float* inL, const float* inR,
                                 float* outL, float* outR, int frames, int sample_rate);

// Process single stereo frame (for optimized embedded use)
void fx_stereo_widen_process_frame(FXStereoWiden* fx, float* left, float* right, int sample_rate);

// Interleaved in-place processor for DPF helper
void fx_stereo_widen_process_interleaved(FXStereoWiden* fx, float* interleavedLR,
                                         int frames, int sample_rate);

// ============================================================================
// Generic Parameter Interface (for wrapper use)
// ============================================================================

int fx_stereo_widen_get_parameter_count(void);
float fx_stereo_widen_get_parameter_value(FXStereoWiden* fx, int index);
void fx_stereo_widen_set_parameter_value(FXStereoWiden* fx, int index, float value);
const char* fx_stereo_widen_get_parameter_name(int index);
const char* fx_stereo_widen_get_parameter_label(int index);
float fx_stereo_widen_get_parameter_default(int index);
float fx_stereo_widen_get_parameter_min(int index);
float fx_stereo_widen_get_parameter_max(int index);
int fx_stereo_widen_get_parameter_group(int index);
const char* fx_stereo_widen_get_group_name(int group);
int fx_stereo_widen_parameter_is_integer(int index);

#ifdef __cplusplus
}
#endif

#endif // FX_STEREO_WIDEN_H
