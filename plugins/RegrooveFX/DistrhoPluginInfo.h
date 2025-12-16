/*
 * RegrooveFX Plugin Info
 * Copyright (C) 2024
 *
 * Permission to use, copy, modify, and/or distribute this software for any purpose with
 * or without fee is hereby granted, provided that the above copyright notice and this
 * permission notice appear in all copies.
 */

#ifndef DISTRHO_PLUGIN_INFO_H_INCLUDED
#define DISTRHO_PLUGIN_INFO_H_INCLUDED

#define DISTRHO_PLUGIN_BRAND "Regroove"
#define DISTRHO_PLUGIN_NAME  "RegrooveFX"
#define DISTRHO_PLUGIN_URI   "https://github.com/gbraad/rfx"

// Build configuration: set RFX_BUILD_UI=1 for UI version, 0 for DSP-only
#ifndef RFX_BUILD_UI
#define RFX_BUILD_UI 1
#endif

#define DISTRHO_PLUGIN_HAS_UI        RFX_BUILD_UI
#define DISTRHO_PLUGIN_IS_RT_SAFE       1
#define DISTRHO_PLUGIN_NUM_INPUTS       2
#define DISTRHO_PLUGIN_NUM_OUTPUTS      2
#define DISTRHO_PLUGIN_WANT_PROGRAMS    0
#define DISTRHO_PLUGIN_WANT_STATE       1
#define DISTRHO_PLUGIN_WANT_FULL_STATE  1
#define DISTRHO_PLUGIN_WANT_TIMEPOS     0

#define DISTRHO_PLUGIN_LV2_CATEGORY "lv2:FilterPlugin"
#define DISTRHO_PLUGIN_VST3_CATEGORIES "Fx|Filter|Distortion|Delay|EQ|Dynamics"

#if RFX_BUILD_UI
// UI settings - using DPF-Widgets DearImGui integration
#define DISTRHO_UI_USE_NANOVG 0
#define DISTRHO_UI_CUSTOM_INCLUDE_PATH "DearImGui/imgui.h"
#define DISTRHO_UI_CUSTOM_WIDGET_TYPE DGL_NAMESPACE::ImGuiTopLevelWidget
#define DISTRHO_UI_FILE_BROWSER 0
#define DISTRHO_UI_DEFAULT_WIDTH  1275
#define DISTRHO_UI_DEFAULT_HEIGHT 390
#endif

enum Parameters {
    // Distortion
    kParameterDistortionEnabled = 0,
    kParameterDistortionDrive,
    kParameterDistortionMix,

    // Filter
    kParameterFilterEnabled,
    kParameterFilterCutoff,
    kParameterFilterResonance,

    // EQ
    kParameterEQEnabled,
    kParameterEQLow,
    kParameterEQMid,
    kParameterEQHigh,

    // Compressor
    kParameterCompressorEnabled,
    kParameterCompressorThreshold,
    kParameterCompressorRatio,
    kParameterCompressorAttack,
    kParameterCompressorRelease,
    kParameterCompressorMakeup,

    // Delay
    kParameterDelayEnabled,
    kParameterDelayTime,
    kParameterDelayFeedback,
    kParameterDelayMix,

    kParameterCount
};

#endif // DISTRHO_PLUGIN_INFO_H_INCLUDED
