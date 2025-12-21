/*
 * RG101 - Regroove SH-101 Style Monophonic Synthesizer
 * DISTRHO Plugin Framework (DPF)
 */

#ifndef DISTRHO_PLUGIN_INFO_H_INCLUDED
#define DISTRHO_PLUGIN_INFO_H_INCLUDED

// Display strings
#define RG101_DISPLAY_NAME "RG101 - Regroove Monophonic Synthesizer"
#define RG101_DESCRIPTION "Roland SH-101 Style Monophonic Bass Synthesizer"
#define RG101_WINDOW_TITLE "RG101 Synthesizer"

#define DISTRHO_PLUGIN_BRAND "Regroove"
#define DISTRHO_PLUGIN_NAME  RG101_DISPLAY_NAME
#define DISTRHO_PLUGIN_URI   "https://https://music.gbraad.nl/regrooved/rg101"

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
#define DISTRHO_UI_DEFAULT_WIDTH    800
#define DISTRHO_UI_DEFAULT_HEIGHT   400

// Parameters
enum Parameters {
    // Oscillator Section
    kParameterSawLevel = 0,
    kParameterSquareLevel,
    kParameterSubLevel,
    kParameterNoiseLevel,
    kParameterPulseWidth,
    kParameterPWMDepth,

    // Filter Section
    kParameterCutoff,
    kParameterResonance,
    kParameterEnvMod,
    kParameterKeyboardTracking,

    // Envelope Section (Filter)
    kParameterFilterAttack,
    kParameterFilterDecay,
    kParameterFilterSustain,
    kParameterFilterRelease,

    // Envelope Section (Amp)
    kParameterAmpAttack,
    kParameterAmpDecay,
    kParameterAmpSustain,
    kParameterAmpRelease,

    // Modulation Section
    kParameterLFOWaveform,
    kParameterLFORate,
    kParameterLFOPitchDepth,
    kParameterLFOFilterDepth,
    kParameterLFOAmpDepth,

    // Performance
    kParameterVelocitySensitivity,
    kParameterPortamento,
    kParameterVolume,

    kParameterCount
};

#endif // DISTRHO_PLUGIN_INFO_H_INCLUDED
