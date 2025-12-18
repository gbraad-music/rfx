/*
 * Regroove Reverb Effect
 * Algorithmic reverb with room size, damping, and mix controls
 */

#ifndef FX_REVERB_H
#define FX_REVERB_H

#include "fx_common.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct FXReverb FXReverb;

// Lifecycle
FXReverb* fx_reverb_create(void);
void fx_reverb_destroy(FXReverb* fx);
void fx_reverb_reset(FXReverb* fx);

// Processing
void fx_reverb_process_f32(FXReverb* fx, float* buffer, int frames, int sample_rate);
void fx_reverb_process_i16(FXReverb* fx, int16_t* buffer, int frames, int sample_rate);
void fx_reverb_process_frame(FXReverb* fx, float* left, float* right, int sample_rate);

// Parameters (0.0 - 1.0)
void fx_reverb_set_enabled(FXReverb* fx, int enabled);
void fx_reverb_set_size(FXReverb* fx, float size);         // Room size
void fx_reverb_set_damping(FXReverb* fx, float damping);   // High frequency damping
void fx_reverb_set_mix(FXReverb* fx, float mix);           // Dry/wet mix

int fx_reverb_get_enabled(FXReverb* fx);
float fx_reverb_get_size(FXReverb* fx);
float fx_reverb_get_damping(FXReverb* fx);
float fx_reverb_get_mix(FXReverb* fx);

#ifdef __cplusplus
}
#endif

#endif // FX_REVERB_H
