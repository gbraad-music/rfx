/*
 * Regroove Low Frequency Oscillator Implementation
 */

#include "synth_lfo.h"
#include <stdlib.h>
#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#ifndef M_2PI
#define M_2PI 6.28318530717958647692
#endif

struct SynthLFO {
    SynthLFOWaveform waveform;
    float frequency;  // Hz
    float phase;      // 0.0 to 1.0
    float random_value;
    int random_counter;
};

SynthLFO* synth_lfo_create(void)
{
    SynthLFO* lfo = (SynthLFO*)malloc(sizeof(SynthLFO));
    if (!lfo) return NULL;

    lfo->waveform = SYNTH_LFO_SINE;
    lfo->frequency = 1.0f;
    lfo->phase = 0.0f;
    lfo->random_value = 0.0f;
    lfo->random_counter = 0;

    return lfo;
}

void synth_lfo_destroy(SynthLFO* lfo)
{
    if (lfo) free(lfo);
}

void synth_lfo_reset(SynthLFO* lfo)
{
    if (!lfo) return;
    lfo->phase = 0.0f;
    lfo->random_value = 0.0f;
    lfo->random_counter = 0;
}

void synth_lfo_set_waveform(SynthLFO* lfo, SynthLFOWaveform waveform)
{
    if (lfo) lfo->waveform = waveform;
}

void synth_lfo_set_frequency(SynthLFO* lfo, float freq_hz)
{
    if (!lfo) return;
    if (freq_hz < 0.01f) freq_hz = 0.01f;
    if (freq_hz > 100.0f) freq_hz = 100.0f;
    lfo->frequency = freq_hz;
}

float synth_lfo_process(SynthLFO* lfo, int sample_rate)
{
    if (!lfo) return 0.0f;

    float output = 0.0f;

    switch (lfo->waveform) {
    case SYNTH_LFO_SINE:
        output = sinf(M_2PI * lfo->phase);
        break;

    case SYNTH_LFO_TRIANGLE:
        if (lfo->phase < 0.5f) {
            output = 4.0f * lfo->phase - 1.0f;
        } else {
            output = 3.0f - 4.0f * lfo->phase;
        }
        break;

    case SYNTH_LFO_SQUARE:
        output = (lfo->phase < 0.5f) ? 1.0f : -1.0f;
        break;

    case SYNTH_LFO_SAW_UP:
        output = 2.0f * lfo->phase - 1.0f;
        break;

    case SYNTH_LFO_SAW_DOWN:
        output = 1.0f - 2.0f * lfo->phase;
        break;

    case SYNTH_LFO_RANDOM:
        // Sample & hold random
        if (lfo->random_counter <= 0) {
            lfo->random_value = ((float)rand() / RAND_MAX) * 2.0f - 1.0f;
            lfo->random_counter = (int)(sample_rate / lfo->frequency);
        }
        lfo->random_counter--;
        output = lfo->random_value;
        break;
    }

    // Advance phase
    float phase_inc = lfo->frequency / (float)sample_rate;
    lfo->phase += phase_inc;
    if (lfo->phase >= 1.0f) {
        lfo->phase -= 1.0f;
    }

    return output;
}
