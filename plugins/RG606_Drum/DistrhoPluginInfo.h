/*
 * RG606 - Regroove TR-606 Style Drum Machine
 * DISTRHO Plugin Framework (DPF)
 */

#ifndef DISTRHO_PLUGIN_INFO_H_INCLUDED
#define DISTRHO_PLUGIN_INFO_H_INCLUDED

// Display strings
#define RG606_DISPLAY_NAME "RG606 - Regroove TR-606 Drum Machine"
#define RG606_DESCRIPTION "Roland TR-606 Style Analog Drum Synthesizer"
#define RG606_WINDOW_TITLE "RG606 Drum Machine"

#define DISTRHO_PLUGIN_BRAND "Regroove"
#define DISTRHO_PLUGIN_NAME  RG606_DISPLAY_NAME
#define DISTRHO_PLUGIN_URI   "https://https://music.gbraad.nl/regrooved/rg606"

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
#define DISTRHO_UI_DEFAULT_WIDTH    650
#define DISTRHO_UI_DEFAULT_HEIGHT   400

// MIDI Note mapping (GM Drum Map)
#define MIDI_NOTE_BD  36  // Bass Drum
#define MIDI_NOTE_SD  38  // Snare Drum
#define MIDI_NOTE_LT  41  // Low Tom
#define MIDI_NOTE_HT  43  // High Tom
#define MIDI_NOTE_CH  42  // Closed Hi-Hat
#define MIDI_NOTE_OH  46  // Open Hi-Hat
#define MIDI_NOTE_CY  49  // Cymbal

// Parameters
enum Parameters {
    // Bass Drum
    kParameterBDLevel = 0,
    kParameterBDTone,
    kParameterBDDecay,

    // Snare Drum
    kParameterSDLevel,
    kParameterSDTone,
    kParameterSDSnappy,

    // Low Tom
    kParameterLTLevel,
    kParameterLTTuning,

    // High Tom
    kParameterHTLevel,
    kParameterHTTuning,

    // Closed Hi-Hat
    kParameterCHLevel,

    // Open Hi-Hat
    kParameterOHLevel,
    kParameterOHDecay,

    // Cymbal
    kParameterCYLevel,
    kParameterCYTone,

    // Master
    kParameterMasterVolume,

    kParameterCount
};

#endif // DISTRHO_PLUGIN_INFO_H_INCLUDED
