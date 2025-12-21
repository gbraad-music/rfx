/*
 * RG909 - Regroove TR-909 Style Drum Machine
 * DISTRHO Plugin Framework (DPF)
 */

#ifndef DISTRHO_PLUGIN_INFO_H_INCLUDED
#define DISTRHO_PLUGIN_INFO_H_INCLUDED

// Display strings
#define RG909_DISPLAY_NAME "RG909 - Regroove TR-909 Drum Machine"
#define RG909_DESCRIPTION "Roland TR-909 Style Analog Drum Synthesizer"
#define RG909_WINDOW_TITLE "RG909 Drum Machine"

#define DISTRHO_PLUGIN_BRAND "Regroove"
#define DISTRHO_PLUGIN_NAME  RG909_DISPLAY_NAME
#define DISTRHO_PLUGIN_URI   "https://music.gbraad.nl/regrooved/rg909"

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
#define DISTRHO_UI_DEFAULT_HEIGHT   450

// MIDI Note mapping (GM Drum Map)
#define MIDI_NOTE_BD  36  // Bass Drum
#define MIDI_NOTE_SD  38  // Snare Drum
#define MIDI_NOTE_LT  41  // Low Tom
#define MIDI_NOTE_MT  47  // Mid Tom
#define MIDI_NOTE_HT  50  // High Tom
#define MIDI_NOTE_RS  37  // Rimshot
#define MIDI_NOTE_HC  39  // Hand Clap

// Parameters
enum Parameters {
    // Bass Drum
    kParameterBDLevel = 0,
    kParameterBDTune,
    kParameterBDDecay,
    kParameterBDAttack,

    // Snare Drum
    kParameterSDLevel,
    kParameterSDTone,
    kParameterSDSnappy,
    kParameterSDTuning,

    // Low Tom
    kParameterLTLevel,
    kParameterLTTuning,
    kParameterLTDecay,

    // Mid Tom
    kParameterMTLevel,
    kParameterMTTuning,
    kParameterMTDecay,

    // High Tom
    kParameterHTLevel,
    kParameterHTTuning,
    kParameterHTDecay,

    // Rimshot
    kParameterRSLevel,
    kParameterRSTuning,

    // Hand Clap
    kParameterHCLevel,
    kParameterHCTone,

    // Master
    kParameterMasterVolume,

    kParameterCount
};

#endif // DISTRHO_PLUGIN_INFO_H_INCLUDED
