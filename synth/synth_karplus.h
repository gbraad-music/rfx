/*
 * Regroove Karplus-Strong Synthesis
 * Physical modeling of plucked strings
 */

#ifndef SYNTH_KARPLUS_H
#define SYNTH_KARPLUS_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct SynthKarplus SynthKarplus;

// Create/destroy
SynthKarplus* synth_karplus_create(void);
void synth_karplus_destroy(SynthKarplus* ks);

// Parameters
void synth_karplus_set_damping(SynthKarplus* ks, float damping);      // 0.0 - 1.0
void synth_karplus_set_brightness(SynthKarplus* ks, float brightness); // 0.0 - 1.0
void synth_karplus_set_stretch(SynthKarplus* ks, float stretch);       // 0.0 - 1.0
void synth_karplus_set_pick_position(SynthKarplus* ks, float position); // 0.0 - 1.0

// Trigger new note
void synth_karplus_trigger(SynthKarplus* ks, float frequency, float velocity, int sample_rate);

// Release (gradual decay)
void synth_karplus_release(SynthKarplus* ks);

// Check if active
bool synth_karplus_is_active(SynthKarplus* ks);

// Process one sample
float synth_karplus_process(SynthKarplus* ks, int sample_rate);

// Reset
void synth_karplus_reset(SynthKarplus* ks);

#ifdef __cplusplus
}
#endif

#endif // SYNTH_KARPLUS_H
