#pragma once
/**
 * M1 Piano Sample Data
 * Reusable sample data for M1 Piano
 *
 * Sample: C3 (MIDI note 48) at 131.6 Hz
 * Sample rate: 22050 Hz
 * Format: 16-bit mono PCM
 */

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Attack/onset sample (C3 - MIDI note 48)
extern const int16_t m1piano_onset[];
extern const uint32_t m1piano_onset_length;

// Loop/tail sample
extern const int16_t m1piano_tail[];
extern const uint32_t m1piano_tail_length;

// Sample metadata
#define M1PIANO_SAMPLE_RATE 22050
#define M1PIANO_ROOT_NOTE 48     // C3
#define M1PIANO_ROOT_FREQ 131.6f // Hz (measured with aubio)

#ifdef __cplusplus
}
#endif
