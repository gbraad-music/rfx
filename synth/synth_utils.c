/*
 * Regroove Synthesizer Utilities Implementation
 */

#include "synth_common.h"

float synth_midi_to_freq(int note)
{
    // f = 440 * 2^((note - 69) / 12)
    // Using exp(x * ln(2)) instead of powf(2, x) to avoid exp2f
    float exponent = (note - 69) / 12.0f;
    return 440.0f * expf(exponent * 0.69314718056f); // ln(2) = 0.69314718056
}
