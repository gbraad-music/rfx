#ifndef DISTRHO_PLUGIN_INFO_H_INCLUDED
#define DISTRHO_PLUGIN_INFO_H_INCLUDED

#define DISTRHO_PLUGIN_BRAND "Regroove"
#define DISTRHO_PLUGIN_NAME  "RG808 Drum"
#define DISTRHO_PLUGIN_URI   "https://github.com/gbraad/rfx/rg808"

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
#define DISTRHO_PLUGIN_VST3_CATEGORIES "Instrument|Synth|Drum"

// UI settings - DearImGui integration
#define DISTRHO_UI_USE_NANOVG 0
#define DISTRHO_UI_CUSTOM_INCLUDE_PATH "DearImGui/imgui.h"
#define DISTRHO_UI_CUSTOM_WIDGET_TYPE DGL_NAMESPACE::ImGuiTopLevelWidget
#define DISTRHO_UI_FILE_BROWSER 0
#define DISTRHO_UI_DEFAULT_WIDTH  500
#define DISTRHO_UI_DEFAULT_HEIGHT 400

// Display strings
#define RG808_DISPLAY_NAME "RG808 Drum Machine"
#define RG808_DESCRIPTION "Regroove 808 Drum Machine - GM MIDI Drum Mapping"
#define RG808_WINDOW_TITLE "RG808 Drum Machine"

// GM MIDI Drum Note Mapping (Standard)
// Note: We'll use the most common 808 sounds
enum DrumNotes {
    kDrumKick = 36,      // Bass Drum 1 (C2)
    kDrumSnare = 38,     // Acoustic Snare (D2)
    kDrumClosedHat = 42, // Closed Hi-Hat (F#2)
    kDrumOpenHat = 46,   // Open Hi-Hat (A#2)
    kDrumClap = 39,      // Hand Clap (D#2)
    kDrumLowTom = 45,    // Low Tom (A2)
    kDrumMidTom = 47,    // Low-Mid Tom (B2)
    kDrumHighTom = 50,   // High Tom (D3)
    kDrumCowbell = 56,   // Cowbell (G#3)
    kDrumRimshot = 37    // Side Stick/Rimshot (C#2)
};

enum Parameters {
    // Kick parameters
    kParameterKickLevel = 0,
    kParameterKickTune,
    kParameterKickDecay,

    // Snare parameters
    kParameterSnareLevel,
    kParameterSnareTune,
    kParameterSnareSnappy,

    // Hi-hat parameters
    kParameterHiHatLevel,
    kParameterHiHatDecay,

    // Clap parameters
    kParameterClapLevel,

    // Tom parameters
    kParameterTomLevel,
    kParameterTomTune,

    // Master
    kParameterMasterLevel,

    kParameterCount
};

#endif
