/*
 * RegrooveM1 Plugin Info - MODEL 1 Mixer Channel Strip
 * Copyright (C) 2024
 *
 * Based on PlayDifferently MODEL 1 DJ Mixer
 * Features: LPF, HPF, Sculpt (semi-parametric EQ)
 */

#ifndef DISTRHO_PLUGIN_INFO_H_INCLUDED
#define DISTRHO_PLUGIN_INFO_H_INCLUDED

#define DISTRHO_PLUGIN_BRAND "Regroove"
#define DISTRHO_PLUGIN_NAME  "RegrooveM1"
#define DISTRHO_PLUGIN_URI   "https://github.com/gbraad/rfx/RegrooveM1"

#define DISTRHO_PLUGIN_HAS_UI        1
#define DISTRHO_PLUGIN_IS_RT_SAFE    1
#define DISTRHO_PLUGIN_NUM_INPUTS    2
#define DISTRHO_PLUGIN_NUM_OUTPUTS   2
#define DISTRHO_PLUGIN_WANT_PROGRAMS 0
#define DISTRHO_PLUGIN_WANT_STATE    1
#define DISTRHO_PLUGIN_WANT_FULL_STATE 1
#define DISTRHO_PLUGIN_WANT_TIMEPOS  0

#define DISTRHO_PLUGIN_LV2_CATEGORY "lv2:MixerPlugin"
#define DISTRHO_PLUGIN_VST3_CATEGORIES "Fx|Filter|EQ"

// UI settings - using DPF-Widgets DearImGui integration
#define DISTRHO_UI_USE_NANOVG 0
#define DISTRHO_UI_CUSTOM_INCLUDE_PATH "DearImGui/imgui.h"
#define DISTRHO_UI_CUSTOM_WIDGET_TYPE DGL_NAMESPACE::ImGuiTopLevelWidget
#define DISTRHO_UI_FILE_BROWSER 0
#define DISTRHO_UI_DEFAULT_WIDTH  200
#define DISTRHO_UI_DEFAULT_HEIGHT 885

enum Parameters {
    // CONTOUR (LPF) - top
    kParameterLPFCutoff = 0,

    // Sculpt Frequency - second
    kParameterSculptFrequency,

    // Sculpt Cut/Boost - third
    kParameterSculptGain,

    // CONTOUR (HPF) - bottom
    kParameterHPFCutoff,

    kParameterCount
};

#endif // DISTRHO_PLUGIN_INFO_H_INCLUDED
