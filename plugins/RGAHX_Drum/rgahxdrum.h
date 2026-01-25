/*
 * RGAHX Drum - AHX One-Shot Drum Synthesizer
 */

#ifndef RGAHX_DRUM_H
#define RGAHX_DRUM_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct RGAHXDrum RGAHXDrum;

// Create/destroy drum synth
RGAHXDrum* rgahxdrum_create(uint32_t sample_rate);
void rgahxdrum_destroy(RGAHXDrum* drum);

// Trigger drum sound
void rgahxdrum_trigger(RGAHXDrum* drum, uint8_t midi_note, uint8_t velocity);

// Process audio (mono output)
void rgahxdrum_process(RGAHXDrum* drum, float* output, uint32_t num_samples);

#ifdef __cplusplus
}
#endif

#endif // RGAHX_DRUM_H
