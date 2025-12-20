#ifndef DISTRHO_PLUGIN_INFO_H_INCLUDED
#define DISTRHO_PLUGIN_INFO_H_INCLUDED

#define DISTRHO_PLUGIN_BRAND "Regroove"
#define DISTRHO_PLUGIN_NAME  "RG303 Synth"
#define DISTRHO_PLUGIN_URI   "https://github.com/gbraad/rfx/rg303"

#define DISTRHO_PLUGIN_HAS_UI        1
#define DISTRHO_PLUGIN_IS_RT_SAFE    1
#define DISTRHO_PLUGIN_IS_SYNTH      1
#define DISTRHO_PLUGIN_NUM_INPUTS    0
#define DISTRHO_PLUGIN_NUM_OUTPUTS   2
#define DISTRHO_PLUGIN_WANT_PROGRAMS 0
#define DISTRHO_PLUGIN_WANT_STATE    1
#define DISTRHO_PLUGIN_WANT_FULL_STATE  1
#define DISTRHO_PLUGIN_WANT_MIDI_INPUT  1
#define DISTRHO_PLUGIN_WANT_MIDI_OUTPUT 0
#define DISTRHO_PLUGIN_WANT_TIMEPOS  0

#define DISTRHO_PLUGIN_LV2_CATEGORY "lv2:InstrumentPlugin"
#define DISTRHO_PLUGIN_VST3_CATEGORIES "Instrument|Synth"

// UI settings - DearImGui integration
#define DISTRHO_UI_USE_NANOVG 0
#define DISTRHO_UI_CUSTOM_INCLUDE_PATH "DearImGui/imgui.h"
#define DISTRHO_UI_CUSTOM_WIDGET_TYPE DGL_NAMESPACE::ImGuiTopLevelWidget
#define DISTRHO_UI_FILE_BROWSER 0
#define DISTRHO_UI_DEFAULT_WIDTH  400
#define DISTRHO_UI_DEFAULT_HEIGHT 450

// Display strings
#define RG303_DISPLAY_NAME "RG303 - Regroove Monophonic Bass Synthesizer"
#define RG303_DESCRIPTION "Regroove Monophonic Bass Synthesizer"
#define RG303_WINDOW_TITLE "RG303 Synthesizer"

enum Parameters {
    kParameterWaveform = 0,
    kParameterCutoff,
    kParameterResonance,
    kParameterEnvMod,
    kParameterDecay,
    kParameterAccent,
    kParameterSlideTime,
    kParameterVolume,
    kParameterCount
};

#endif
