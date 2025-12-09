/*
 * Regroove Compressor Effect
 * Professional RMS compressor with soft knee and makeup gain
 */

#ifndef FX_COMPRESSOR_H
#define FX_COMPRESSOR_H

#include "fx_common.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct FXCompressor FXCompressor;

// Lifecycle
FXCompressor* fx_compressor_create(void);
void fx_compressor_destroy(FXCompressor* fx);
void fx_compressor_reset(FXCompressor* fx);

// Processing
void fx_compressor_process_f32(FXCompressor* fx, float* buffer, int frames, int sample_rate);
void fx_compressor_process_i16(FXCompressor* fx, int16_t* buffer, int frames, int sample_rate);
void fx_compressor_process_frame(FXCompressor* fx, float* left, float* right, int sample_rate);

// Parameters (0.0 - 1.0)
void fx_compressor_set_enabled(FXCompressor* fx, int enabled);
void fx_compressor_set_threshold(FXCompressor* fx, float threshold);  // 0.0-1.0 maps to -40dB to -6dB
void fx_compressor_set_ratio(FXCompressor* fx, float ratio);          // 0.0-1.0 maps to 1:1 to 20:1
void fx_compressor_set_attack(FXCompressor* fx, float attack);        // 0.0-1.0 maps to 0.5ms to 50ms
void fx_compressor_set_release(FXCompressor* fx, float release);      // 0.0-1.0 maps to 10ms to 500ms
void fx_compressor_set_makeup(FXCompressor* fx, float makeup);        // 0.0-1.0 maps to 0.125x to 8x

int fx_compressor_get_enabled(FXCompressor* fx);
float fx_compressor_get_threshold(FXCompressor* fx);
float fx_compressor_get_ratio(FXCompressor* fx);
float fx_compressor_get_attack(FXCompressor* fx);
float fx_compressor_get_release(FXCompressor* fx);
float fx_compressor_get_makeup(FXCompressor* fx);

#ifdef __cplusplus
}
#endif

#endif // FX_COMPRESSOR_H
