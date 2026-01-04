/*
 * RG1Piano - Regroove M1 Piano Sampler
 * DISTRHO Plugin Framework (DPF)
 */

#ifndef DISTRHO_PLUGIN_INFO_H_INCLUDED
#define DISTRHO_PLUGIN_INFO_H_INCLUDED

// Display strings
#define RG1PIANO_DISPLAY_NAME "RG1Piano - M1 Piano Sampler"
#define RG1PIANO_DESCRIPTION "M1 Piano Sample-based Synthesizer"
#define RG1PIANO_WINDOW_TITLE "RG1Piano Sampler"

#define DISTRHO_PLUGIN_BRAND "Regroove"
#define DISTRHO_PLUGIN_NAME  RG1PIANO_DISPLAY_NAME
#define DISTRHO_PLUGIN_URI   "https://music.gbraad.nl/regrooved/rg1piano"

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
#define DISTRHO_UI_DEFAULT_WIDTH    500
#define DISTRHO_UI_DEFAULT_HEIGHT   350

// Parameters
enum Parameters {
    kParameterDecay = 0,
    kParameterResonance,
    kParameterBrightness,
    kParameterVelocitySens,
    kParameterVolume,
    kParameterLfoRate,
    kParameterLfoDepth,

    kParameterCount
};

#endif // DISTRHO_PLUGIN_INFO_H_INCLUDED
