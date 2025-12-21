/*
 * RGK1 - Regroove Kawai K1 Style PCM Synthesizer
 * DISTRHO Plugin Framework (DPF)
 */

#ifndef DISTRHO_PLUGIN_INFO_H_INCLUDED
#define DISTRHO_PLUGIN_INFO_H_INCLUDED

// Display strings
#define RGK1_DISPLAY_NAME "RGK1 - Regroove Kawai K1 Style Synthesizer"
#define RGK1_DESCRIPTION "Kawai K1 Style Dual DCO/DCF PCM Synthesizer"
#define RGK1_WINDOW_TITLE "RGK1 Synthesizer"

#define DISTRHO_PLUGIN_BRAND "Regroove"
#define DISTRHO_PLUGIN_NAME  RGK1_DISPLAY_NAME
#define DISTRHO_PLUGIN_URI   "https://music.gbraad.nl/regrooved/rgk1"

#define DISTRHO_PLUGIN_HAS_UI        1
#define DISTRHO_PLUGIN_IS_RT_SAFE    1
#define DISTRHO_PLUGIN_IS_SYNTH      1
#define DISTRHO_PLUGIN_NUM_INPUTS    0
#define DISTRHO_PLUGIN_NUM_OUTPUTS   2
#define DISTRHO_PLUGIN_WANT_PROGRAMS 0
#define DISTRHO_PLUGIN_WANT_STATE    0
#define DISTRHO_PLUGIN_WANT_TIMEPOS  0
#define DISTRHO_PLUGIN_WANT_MIDI_INPUT 1

// UI
#define DISTRHO_UI_USE_NANOVG       0
#define DISTRHO_UI_USER_RESIZABLE   1
#define DISTRHO_UI_DEFAULT_WIDTH    900
#define DISTRHO_UI_DEFAULT_HEIGHT   550

// Wavetable size for PCM waveforms
#define WAVETABLE_SIZE 128

// Parameters
enum Parameters {
    // DCO 1
    kParameterDCO1Wave = 0,
    kParameterDCO1Octave,
    kParameterDCO1Level,
    kParameterDCO1Detune,

    // DCO 2
    kParameterDCO2Wave,
    kParameterDCO2Octave,
    kParameterDCO2Level,
    kParameterDCO2Detune,

    // DCF 1 (for DCO1)
    kParameterDCF1Cutoff,
    kParameterDCF1Resonance,
    kParameterDCF1EnvDepth,
    kParameterDCF1KeyTrack,

    // DCF 2 (for DCO2)
    kParameterDCF2Cutoff,
    kParameterDCF2Resonance,
    kParameterDCF2EnvDepth,
    kParameterDCF2KeyTrack,

    // Filter/Pitch Envelope
    kParameterFiltAttack,
    kParameterFiltDecay,
    kParameterFiltSustain,
    kParameterFiltRelease,

    // Amplitude Envelope
    kParameterAmpAttack,
    kParameterAmpDecay,
    kParameterAmpSustain,
    kParameterAmpRelease,

    // LFO
    kParameterLFOWave,
    kParameterLFORate,
    kParameterLFOPitchDepth,
    kParameterLFOFilterDepth,
    kParameterLFOAmpDepth,

    // Master
    kParameterVelocitySensitivity,
    kParameterVolume,

    kParameterCount
};

#endif // DISTRHO_PLUGIN_INFO_H_INCLUDED
