/*
 * RG560 - Regroove PSS-560 Style FM Synthesizer
 * DISTRHO Plugin Framework (DPF)
 */

#ifndef DISTRHO_PLUGIN_INFO_H_INCLUDED
#define DISTRHO_PLUGIN_INFO_H_INCLUDED

// Display strings
#define RG560_DISPLAY_NAME "RG560 - Regroove FM Synthesizer"
#define RG560_DESCRIPTION "Yamaha PSS-560 Style 2-Operator FM Synthesizer"
#define RG560_WINDOW_TITLE "RG560 Synthesizer"

#define DISTRHO_PLUGIN_BRAND "Regroove"
#define DISTRHO_PLUGIN_NAME  RG560_DISPLAY_NAME
#define DISTRHO_PLUGIN_URI   "https://https://music.gbraad.nl/regrooved/rg560"

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
#define DISTRHO_UI_DEFAULT_WIDTH    700
#define DISTRHO_UI_DEFAULT_HEIGHT   450

// Parameters
enum Parameters {
    // Algorithm
    kParameterAlgorithm = 0,  // 0=FM, 1=Additive

    // Operator 1 (Carrier)
    kParameterOp1Multiplier,
    kParameterOp1Level,
    kParameterOp1Attack,
    kParameterOp1Decay,
    kParameterOp1Sustain,
    kParameterOp1Release,
    kParameterOp1Waveform,

    // Operator 2 (Modulator)
    kParameterOp2Multiplier,
    kParameterOp2Level,
    kParameterOp2Attack,
    kParameterOp2Decay,
    kParameterOp2Sustain,
    kParameterOp2Release,
    kParameterOp2Waveform,

    // Modulation
    kParameterFeedback,
    kParameterDetune,
    kParameterLFORate,
    kParameterLFOPitchDepth,
    kParameterLFOAmpDepth,

    // Master
    kParameterVelocitySensitivity,
    kParameterVolume,

    kParameterCount
};

#endif // DISTRHO_PLUGIN_INFO_H_INCLUDED
