#pragma once
/**
 * M1 Piano Sample Data
 */

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Attack/onset sample (MIDI note 56 - G#3)
extern const int16_t m1piano_onset[];
extern const uint32_t m1piano_onset_length;

// Loop/tail sample
extern const int16_t m1piano_tail[];
extern const uint32_t m1piano_tail_length;

#ifdef __cplusplus
}
#endif
