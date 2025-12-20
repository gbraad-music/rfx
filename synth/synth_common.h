/*
 * Common definitions for Regroove Synthesizer Components
 * Copyright (C) 2024
 * SPDX-License-Identifier: ISC
 */

#ifndef SYNTH_COMMON_H
#define SYNTH_COMMON_H

#include <stdint.h>
#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#ifndef M_2PI
#define M_2PI (2.0 * M_PI)
#endif

#ifdef __cplusplus
extern "C" {
#endif

// MIDI note to frequency conversion
float synth_midi_to_freq(int note);

#ifdef __cplusplus
}
#endif

#endif // SYNTH_COMMON_H
