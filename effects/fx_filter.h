/*
 * Regroove Resonant Filter Effect
 * DJ-style low-pass filter with cutoff and resonance
 */

#ifndef FX_FILTER_H
#define FX_FILTER_H

#include "fx_common.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct FXFilter FXFilter;

// Lifecycle
FXFilter* fx_filter_create(void);
void fx_filter_destroy(FXFilter* fx);
void fx_filter_reset(FXFilter* fx);

// Processing
void fx_filter_process_f32(FXFilter* fx, float* buffer, int frames, int sample_rate);
void fx_filter_process_i16(FXFilter* fx, int16_t* buffer, int frames, int sample_rate);
void fx_filter_process_frame(FXFilter* fx, float* left, float* right, int sample_rate);

// Parameters (0.0 - 1.0)
void fx_filter_set_enabled(FXFilter* fx, int enabled);
void fx_filter_set_cutoff(FXFilter* fx, float cutoff);
void fx_filter_set_resonance(FXFilter* fx, float resonance);

int fx_filter_get_enabled(FXFilter* fx);
float fx_filter_get_cutoff(FXFilter* fx);
float fx_filter_get_resonance(FXFilter* fx);

#ifdef __cplusplus
}
#endif

#endif // FX_FILTER_H
