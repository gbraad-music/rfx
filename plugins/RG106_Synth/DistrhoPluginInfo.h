/*
 * RG106 - Regroove Juno-106 Style Polyphonic Synthesizer
 * DISTRHO Plugin Framework (DPF)
 */

#ifndef DISTRHO_PLUGIN_INFO_H_INCLUDED
#define DISTRHO_PLUGIN_INFO_H_INCLUDED

// Display strings
#define RG106_DISPLAY_NAME "RG106 - Regroove Juno-106 Style Synthesizer"
#define RG106_DESCRIPTION "Roland Juno-106 Style 6-Voice Polyphonic Synthesizer"
#define RG106_WINDOW_TITLE "RG106 Synthesizer"

#define DISTRHO_PLUGIN_BRAND "Regroove"
#define DISTRHO_PLUGIN_NAME  RG106_DISPLAY_NAME
#define DISTRHO_PLUGIN_URI   "https://https://music.gbraad.nl/regrooved/rg106"

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
#define DISTRHO_UI_DEFAULT_HEIGHT   500

// Parameters
enum Parameters {
    // DCO
    kParameterPulseWidth = 0,
    kParameterPWM,
    kParameterSubLevel,

    // VCF
    kParameterCutoff,
    kParameterResonance,
    kParameterEnvMod,
    kParameterLFOMod,
    kParameterKeyboardTracking,

    // HPF
    kParameterHPFCutoff,

    // VCA
    kParameterVCALevel,

    // Envelope
    kParameterAttack,
    kParameterDecay,
    kParameterSustain,
    kParameterRelease,

    // LFO
    kParameterLFOWaveform,
    kParameterLFORate,
    kParameterLFODelay,
    kParameterLFOPitchDepth,
    kParameterLFOAmpDepth,

    // Chorus
    kParameterChorusMode,     // 0=off, 0.5=I, 1=II
    kParameterChorusRate,
    kParameterChorusDepth,

    // Performance
    kParameterVelocitySensitivity,
    kParameterPortamento,

    // Master
    kParameterVolume,

    kParameterCount
};

#endif // DISTRHO_PLUGIN_INFO_H_INCLUDED
