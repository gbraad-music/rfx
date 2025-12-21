/*
 * Regroove Limiter Effect
 * Brick-wall limiter with lookahead and soft clipping
 */

#ifndef FX_LIMITER_H
#define FX_LIMITER_H

#include "fx_common.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct FXLimiter FXLimiter;

// Lifecycle
FXLimiter* fx_limiter_create(void);
void fx_limiter_destroy(FXLimiter* fx);
void fx_limiter_reset(FXLimiter* fx);

// Processing
void fx_limiter_process_f32(FXLimiter* fx, float* buffer, int frames, int sample_rate);
void fx_limiter_process_i16(FXLimiter* fx, int16_t* buffer, int frames, int sample_rate);
void fx_limiter_process_frame(FXLimiter* fx, float* left, float* right, int sample_rate);

// Parameters (0.0 - 1.0)
void fx_limiter_set_enabled(FXLimiter* fx, int enabled);
void fx_limiter_set_threshold(FXLimiter* fx, float threshold);  // 0.0-1.0 maps to -20dB to 0dB
void fx_limiter_set_release(FXLimiter* fx, float release);      // 0.0-1.0 maps to 10ms to 1000ms
void fx_limiter_set_ceiling(FXLimiter* fx, float ceiling);      // 0.0-1.0 maps to -6dB to 0dB
void fx_limiter_set_lookahead(FXLimiter* fx, float lookahead);  // 0.0-1.0 maps to 0ms to 10ms

int fx_limiter_get_enabled(FXLimiter* fx);
float fx_limiter_get_threshold(FXLimiter* fx);
float fx_limiter_get_release(FXLimiter* fx);
float fx_limiter_get_ceiling(FXLimiter* fx);
float fx_limiter_get_lookahead(FXLimiter* fx);

// Metering
float fx_limiter_get_gain_reduction(FXLimiter* fx);  // Current gain reduction in dB

#ifdef __cplusplus
}
#endif

#endif // FX_LIMITER_H
