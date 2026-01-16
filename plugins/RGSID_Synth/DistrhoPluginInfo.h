#ifndef DISTRHO_PLUGIN_INFO_H_INCLUDED
#define DISTRHO_PLUGIN_INFO_H_INCLUDED

// Display strings
#define RGSID_DISPLAY_NAME "RGSID - Regroove SID Synthesizer"
#define RGSID_DESCRIPTION "Commodore 64 SID Chip Emulation"
#define RGSID_WINDOW_TITLE "RGSID Synthesizer"

#define DISTRHO_PLUGIN_BRAND "Regroove"
#define DISTRHO_PLUGIN_NAME  RGSID_DISPLAY_NAME
#define DISTRHO_PLUGIN_URI   "https://music.gbraad.nl/regrooved/rgsid"

#define DISTRHO_PLUGIN_HAS_UI        1
#define DISTRHO_PLUGIN_IS_RT_SAFE    1
#define DISTRHO_PLUGIN_IS_SYNTH      1
#define DISTRHO_PLUGIN_NUM_INPUTS    0
#define DISTRHO_PLUGIN_NUM_OUTPUTS   2
#define DISTRHO_PLUGIN_WANT_PROGRAMS 1
#define DISTRHO_PLUGIN_WANT_STATE    0
#define DISTRHO_PLUGIN_WANT_FULL_STATE  0
#define DISTRHO_PLUGIN_WANT_MIDI_INPUT  1
#define DISTRHO_PLUGIN_WANT_MIDI_OUTPUT 1
#define DISTRHO_PLUGIN_WANT_TIMEPOS  0

#define DISTRHO_PLUGIN_LV2_CATEGORY "lv2:InstrumentPlugin"
#define DISTRHO_PLUGIN_VST3_CATEGORIES "Instrument|Synth"

// UI settings - DearImGui integration
#define DISTRHO_UI_USE_NANOVG 0
#define DISTRHO_UI_CUSTOM_INCLUDE_PATH "DearImGui/imgui.h"
#define DISTRHO_UI_CUSTOM_WIDGET_TYPE DGL_NAMESPACE::ImGuiTopLevelWidget
#define DISTRHO_UI_FILE_BROWSER 0
#define DISTRHO_UI_DEFAULT_WIDTH  500
#define DISTRHO_UI_DEFAULT_HEIGHT 550

// Parameters (per-voice + global)
enum Parameters {
    // Voice 1
    kParameterVoice1Waveform = 0,
    kParameterVoice1PulseWidth,
    kParameterVoice1Attack,
    kParameterVoice1Decay,
    kParameterVoice1Sustain,
    kParameterVoice1Release,
    kParameterVoice1RingMod,
    kParameterVoice1Sync,

    // Voice 2
    kParameterVoice2Waveform,
    kParameterVoice2PulseWidth,
    kParameterVoice2Attack,
    kParameterVoice2Decay,
    kParameterVoice2Sustain,
    kParameterVoice2Release,
    kParameterVoice2RingMod,
    kParameterVoice2Sync,

    // Voice 3
    kParameterVoice3Waveform,
    kParameterVoice3PulseWidth,
    kParameterVoice3Attack,
    kParameterVoice3Decay,
    kParameterVoice3Sustain,
    kParameterVoice3Release,
    kParameterVoice3RingMod,
    kParameterVoice3Sync,

    // Filter
    kParameterFilterMode,
    kParameterFilterCutoff,
    kParameterFilterResonance,
    kParameterFilterVoice1,
    kParameterFilterVoice2,
    kParameterFilterVoice3,

    // Global
    kParameterVolume,

    kParameterCount
};

#endif
