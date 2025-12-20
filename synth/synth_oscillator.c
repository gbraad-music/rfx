/*
 * Regroove Synthesizer Oscillator Implementation
 */

#include "synth_oscillator.h"
#include <stdlib.h>
#include <math.h>

struct SynthOscillator {
    SynthOscWaveform waveform;
    float frequency;
    float phase;          // 0.0 to 1.0
    float pulse_width;    // 0.0 to 1.0 for square wave
};

SynthOscillator* synth_oscillator_create(void)
{
    SynthOscillator* osc = (SynthOscillator*)malloc(sizeof(SynthOscillator));
    if (!osc) return NULL;

    osc->waveform = SYNTH_OSC_SAW;
    osc->frequency = 440.0f;
    osc->phase = 0.0f;
    osc->pulse_width = 0.5f;

    return osc;
}

void synth_oscillator_destroy(SynthOscillator* osc)
{
    if (osc) free(osc);
}

void synth_oscillator_reset(SynthOscillator* osc)
{
    if (!osc) return;
    osc->phase = 0.0f;
}

void synth_oscillator_set_waveform(SynthOscillator* osc, SynthOscWaveform waveform)
{
    if (osc) osc->waveform = waveform;
}

void synth_oscillator_set_frequency(SynthOscillator* osc, float freq)
{
    if (osc) osc->frequency = freq;
}

void synth_oscillator_set_pulse_width(SynthOscillator* osc, float width)
{
    if (!osc) return;
    osc->pulse_width = width < 0.0f ? 0.0f : (width > 1.0f ? 1.0f : width);
}

void synth_oscillator_set_phase(SynthOscillator* osc, float phase)
{
    if (!osc) return;
    osc->phase = phase - floorf(phase); // Wrap to 0.0-1.0
}

// PolyBLEP residual for band-limiting
static float polyblep(float phase, float phase_inc)
{
    if (phase < phase_inc) {
        phase /= phase_inc;
        return phase + phase - phase * phase - 1.0f;
    }
    else if (phase > 1.0f - phase_inc) {
        phase = (phase - 1.0f) / phase_inc;
        return phase * phase + phase + phase + 1.0f;
    }
    return 0.0f;
}

float synth_oscillator_process(SynthOscillator* osc, int sample_rate)
{
    if (!osc) return 0.0f;

    float output = 0.0f;
    float phase_inc = osc->frequency / (float)sample_rate;

    switch (osc->waveform) {
    case SYNTH_OSC_SAW: {
        // Band-limited sawtooth using PolyBLEP
        output = 2.0f * osc->phase - 1.0f;
        output -= polyblep(osc->phase, phase_inc);
        break;
    }

    case SYNTH_OSC_SQUARE: {
        // Band-limited square wave using PolyBLEP
        output = osc->phase < osc->pulse_width ? 1.0f : -1.0f;
        output += polyblep(osc->phase, phase_inc);

        // Add BLEP at pulse width transition
        float mod_phase = osc->phase + (1.0f - osc->pulse_width);
        if (mod_phase >= 1.0f) mod_phase -= 1.0f;
        output -= polyblep(mod_phase, phase_inc);
        break;
    }

    case SYNTH_OSC_TRIANGLE: {
        // Triangle wave (integrated square)
        if (osc->phase < 0.5f) {
            output = 4.0f * osc->phase - 1.0f;
        } else {
            output = 3.0f - 4.0f * osc->phase;
        }
        break;
    }

    case SYNTH_OSC_SINE: {
        output = sinf(M_2PI * osc->phase);
        break;
    }
    }

    // Advance phase
    osc->phase += phase_inc;
    if (osc->phase >= 1.0f) {
        osc->phase -= 1.0f;
    }

    return output;
}
