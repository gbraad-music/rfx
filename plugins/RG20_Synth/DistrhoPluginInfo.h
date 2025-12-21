/*
 * RG20 - Regroove MS-20 Style Monophonic Synthesizer
 * DISTRHO Plugin Framework (DPF)
 */

#ifndef DISTRHO_PLUGIN_INFO_H_INCLUDED
#define DISTRHO_PLUGIN_INFO_H_INCLUDED

// Display strings
#define RG20_DISPLAY_NAME "RG20 - Regroove MS-20 Style Synthesizer"
#define RG20_DESCRIPTION "Korg MS-20 Style Monophonic Synthesizer"
#define RG20_WINDOW_TITLE "RG20 Synthesizer"

#define DISTRHO_PLUGIN_BRAND "Regroove"
#define DISTRHO_PLUGIN_NAME  RG20_DISPLAY_NAME
#define DISTRHO_PLUGIN_URI   "https://https://music.gbraad.nl/regrooved/rg20"

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
#define DISTRHO_UI_DEFAULT_HEIGHT   450

// Parameters
enum Parameters {
    // VCO 1
    kParameterVCO1Waveform = 0,
    kParameterVCO1Octave,
    kParameterVCO1Tune,
    kParameterVCO1Level,

    // VCO 2
    kParameterVCO2Waveform,
    kParameterVCO2Octave,
    kParameterVCO2Tune,
    kParameterVCO2Level,
    kParameterVCO2PulseWidth,
    kParameterVCO2PWMDepth,
    kParameterVCOSync,

    // Mixer
    kParameterNoiseLevel,
    kParameterRingModLevel,

    // Highpass Filter
    kParameterHPFCutoff,
    kParameterHPFPeak,

    // Lowpass Filter
    kParameterLPFCutoff,
    kParameterLPFPeak,
    kParameterKeyboardTracking,

    // Filter Envelope (ADR - no sustain)
    kParameterFilterAttack,
    kParameterFilterDecay,
    kParameterFilterRelease,
    kParameterFilterEnvAmount,

    // Amp Envelope (ADR)
    kParameterAmpAttack,
    kParameterAmpDecay,
    kParameterAmpRelease,

    // Modulation
    kParameterLFOWaveform,
    kParameterLFORate,
    kParameterLFOPitchDepth,
    kParameterLFOFilterDepth,

    // Performance
    kParameterVelocitySensitivity,
    kParameterPortamento,
    kParameterVolume,

    kParameterCount
};

#endif // DISTRHO_PLUGIN_INFO_H_INCLUDED
