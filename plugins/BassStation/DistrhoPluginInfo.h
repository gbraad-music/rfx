#ifndef DISTRHO_PLUGIN_INFO_H_INCLUDED
#define DISTRHO_PLUGIN_INFO_H_INCLUDED

// Display strings
#define BASS_STATION_DISPLAY_NAME "Bass Station - Novation Style Synthesizer"
#define BASS_STATION_DESCRIPTION "Novation Bass Station Style Analog Bass Synthesizer"
#define BASS_STATION_WINDOW_TITLE "Bass Station Synthesizer"

#define DISTRHO_PLUGIN_BRAND "Regroove"
#define DISTRHO_PLUGIN_NAME  BASS_STATION_DISPLAY_NAME
#define DISTRHO_PLUGIN_URI   "https://music.gbraad.nl/regrooved/bass-station"

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
#define DISTRHO_UI_DEFAULT_WIDTH  800
#define DISTRHO_UI_DEFAULT_HEIGHT 600

enum Parameters {
    // Oscillator 1
    kParameterOsc1Waveform = 0,
    kParameterOsc1Octave,
    kParameterOsc1Fine,
    kParameterOsc1PW,

    // Oscillator 2
    kParameterOsc2Waveform,
    kParameterOsc2Octave,
    kParameterOsc2Fine,
    kParameterOsc2PW,

    // Oscillator Mix & Sync
    kParameterOscMix,
    kParameterOscSync,

    // Sub-Oscillator
    kParameterSubMode,
    kParameterSubWave,
    kParameterSubLevel,

    // Filter
    kParameterFilterMode,
    kParameterFilterType,
    kParameterFilterCutoff,
    kParameterFilterResonance,
    kParameterFilterDrive,

    // Amp Envelope
    kParameterAmpAttack,
    kParameterAmpDecay,
    kParameterAmpSustain,
    kParameterAmpRelease,

    // Mod Envelope
    kParameterModAttack,
    kParameterModDecay,
    kParameterModSustain,
    kParameterModRelease,

    // Modulation Amounts
    kParameterModEnvToFilter,
    kParameterModEnvToPitch,
    kParameterModEnvToPW,

    // LFO 1
    kParameterLFO1Rate,
    kParameterLFO1Waveform,
    kParameterLFO1ToPitch,

    // LFO 2
    kParameterLFO2Rate,
    kParameterLFO2Waveform,
    kParameterLFO2ToPW,
    kParameterLFO2ToFilter,

    // Performance
    kParameterPortamento,
    kParameterVolume,
    kParameterDistortion,

    kParameterCount
};

#endif
