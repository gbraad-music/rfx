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

#ifdef __cplusplus
}
#endif

#endif // FX_PHASER_H
