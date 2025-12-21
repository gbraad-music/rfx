/*
 * RGDS3 - Regroove Simmons SDS-3 Style Drum Synthesizer
 * DISTRHO Plugin Framework (DPF)
 */

#ifndef DISTRHO_PLUGIN_INFO_H_INCLUDED
#define DISTRHO_PLUGIN_INFO_H_INCLUDED

// Display strings
#define RGDS3_DISPLAY_NAME "RGDS3 - Regroove Simmons SDS-3 Style Drum Synth"
#define RGDS3_DESCRIPTION "Simmons SDS-3 Style Analog Drum Synthesizer"
#define RGDS3_WINDOW_TITLE "RGDS3 Drum Synth"

#define DISTRHO_PLUGIN_BRAND "Regroove"
#define DISTRHO_PLUGIN_NAME  RGDS3_DISPLAY_NAME
#define DISTRHO_PLUGIN_URI   "https://music.gbraad.nl/regrooved/rgds3"

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
#define DISTRHO_UI_DEFAULT_WIDTH    900
#define DISTRHO_UI_DEFAULT_HEIGHT   400

// MIDI Note mapping (GM Drum Map)
#define MIDI_NOTE_BD    36  // Bass Drum (C1)
#define MIDI_NOTE_SD    38  // Snare Drum (D1)
#define MIDI_NOTE_TOM1  48  // Tom 1 (C2)
#define MIDI_NOTE_TOM2  47  // Tom 2 (B1)
#define MIDI_NOTE_TOM3  45  // Tom 3 (A1)

// Parameters
enum Parameters {
    // Bass Drum
    kParameterBDTone = 0,
    kParameterBDBend,
    kParameterBDDecay,
    kParameterBDNoise,
    kParameterBDLevel,

    // Snare Drum
    kParameterSDTone,
    kParameterSDBend,
    kParameterSDDecay,
    kParameterSDNoise,
    kParameterSDLevel,

    // Tom 1
    kParameterT1Tone,
    kParameterT1Bend,
    kParameterT1Decay,
    kParameterT1Noise,
    kParameterT1Level,

    // Tom 2
    kParameterT2Tone,
    kParameterT2Bend,
    kParameterT2Decay,
    kParameterT2Noise,
    kParameterT2Level,

    // Tom 3
    kParameterT3Tone,
    kParameterT3Bend,
    kParameterT3Decay,
    kParameterT3Noise,
    kParameterT3Level,

    // Master
    kParameterVolume,

    kParameterCount
};

#endif // DISTRHO_PLUGIN_INFO_H_INCLUDED
