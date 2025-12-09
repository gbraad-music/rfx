/*
 * Regroove Delay Effect
 * Stereo delay with feedback and mix controls
 */

#ifndef FX_DELAY_H
#define FX_DELAY_H

#include "fx_common.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct FXDelay FXDelay;

// Lifecycle
FXDelay* fx_delay_create(void);
void fx_delay_destroy(FXDelay* fx);
void fx_delay_reset(FXDelay* fx);

// Processing
void fx_delay_process_f32(FXDelay* fx, float* buffer, int frames, int sample_rate);
void fx_delay_process_i16(FXDelay* fx, int16_t* buffer, int frames, int sample_rate);
void fx_delay_process_frame(FXDelay* fx, float* left, float* right, int sample_rate);

// Parameters (0.0 - 1.0)
void fx_delay_set_enabled(FXDelay* fx, int enabled);
void fx_delay_set_time(FXDelay* fx, float time);       // 0.0-1.0 (maps to 10ms-1000ms)
void fx_delay_set_feedback(FXDelay* fx, float feedback);
void fx_delay_set_mix(FXDelay* fx, float mix);

int fx_delay_get_enabled(FXDelay* fx);
float fx_delay_get_time(FXDelay* fx);
float fx_delay_get_feedback(FXDelay* fx);
float fx_delay_get_mix(FXDelay* fx);

#ifdef __cplusplus
}
#endif

#endif // FX_DELAY_H
