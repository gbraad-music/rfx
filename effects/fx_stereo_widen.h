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

void fx_stereo_widen_process_f32(FXStereoWiden* fx, const float* inL, const float* inR,
                                 float* outL, float* outR, int frames, int sample_rate);

// Interleaved in-place processor for DPF helper
void fx_stereo_widen_process_interleaved(FXStereoWiden* fx, float* interleavedLR,
                                         int frames, int sample_rate);

#ifdef __cplusplus
}
#endif

#endif // FX_STEREO_WIDEN_H
