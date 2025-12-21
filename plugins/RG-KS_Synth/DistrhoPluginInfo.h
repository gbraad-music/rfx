/*
 * RG-KS - Regroove Karplus-Strong Synthesizer
 * DISTRHO Plugin Framework (DPF)
 */

#ifndef DISTRHO_PLUGIN_INFO_H_INCLUDED
#define DISTRHO_PLUGIN_INFO_H_INCLUDED

// Display strings
#define RGKS_DISPLAY_NAME "RG-KS - Regroove Karplus-Strong Synth"
#define RGKS_DESCRIPTION "Physical Modeling Plucked String Synthesizer"
#define RGKS_WINDOW_TITLE "RG-KS Synthesizer"

#define DISTRHO_PLUGIN_BRAND "Regroove"
#define DISTRHO_PLUGIN_NAME  RGKS_DISPLAY_NAME
#define DISTRHO_PLUGIN_URI   "https://https://music.gbraad.nl/regrooved/rgks"

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
    kParameterDamping = 0,
    kParameterBrightness,
    kParameterStretch,
    kParameterPickPosition,
    kParameterVolume,

    kParameterCount
};

#endif // DISTRHO_PLUGIN_INFO_H_INCLUDED
