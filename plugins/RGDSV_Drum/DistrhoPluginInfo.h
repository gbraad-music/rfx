/*
 * RGDSV - Regroove Simmons SDS-V Style Drum Synthesizer
 * DISTRHO Plugin Framework (DPF)
 */

#ifndef DISTRHO_PLUGIN_INFO_H_INCLUDED
#define DISTRHO_PLUGIN_INFO_H_INCLUDED

// Display strings
#define RGDSV_DISPLAY_NAME "RGDSV - Regroove Simmons SDS-V Style Drum Synth"
#define RGDSV_DESCRIPTION "Simmons SDS-V Style Advanced Drum Synthesizer"
#define RGDSV_WINDOW_TITLE "RGDSV Drum Synth"

#define DISTRHO_PLUGIN_BRAND "Regroove"
#define DISTRHO_PLUGIN_NAME  RGDSV_DISPLAY_NAME
#define DISTRHO_PLUGIN_URI   "https://music.gbraad.nl/regrooved/rgdsv"

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
#define DISTRHO_UI_DEFAULT_WIDTH    1000
#define DISTRHO_UI_DEFAULT_HEIGHT   450

// MIDI Note mapping (GM Drum Map)
#define MIDI_NOTE_BD    36  // Bass Drum (C1)
#define MIDI_NOTE_SD    38  // Snare Drum (D1)
#define MIDI_NOTE_TOM1  48  // Tom 1 (C2)
#define MIDI_NOTE_TOM2  47  // Tom 2 (B1)
#define MIDI_NOTE_TOM3  45  // Tom 3 (A1)
#define MIDI_NOTE_TOM4  43  // Tom 4 (G1)

// Parameters
enum Parameters {
    // Bass Drum
    kParameterBDWave = 0,
    kParameterBDTone,
    kParameterBDBend,
    kParameterBDDecay,
    kParameterBDClick,
    kParameterBDNoise,
    kParameterBDFilter,
    kParameterBDLevel,

    // Snare Drum
    kParameterSDWave,
    kParameterSDTone,
    kParameterSDBend,
    kParameterSDDecay,
    kParameterSDClick,
    kParameterSDNoise,
    kParameterSDFilter,
    kParameterSDLevel,

    // Tom 1
    kParameterT1Wave,
    kParameterT1Tone,
    kParameterT1Bend,
    kParameterT1Decay,
    kParameterT1Click,
    kParameterT1Noise,
    kParameterT1Filter,
    kParameterT1Level,

    // Tom 2
    kParameterT2Wave,
    kParameterT2Tone,
    kParameterT2Bend,
    kParameterT2Decay,
    kParameterT2Click,
    kParameterT2Noise,
    kParameterT2Filter,
    kParameterT2Level,

    // Tom 3
    kParameterT3Wave,
    kParameterT3Tone,
    kParameterT3Bend,
    kParameterT3Decay,
    kParameterT3Click,
    kParameterT3Noise,
    kParameterT3Filter,
    kParameterT3Level,

    // Tom 4
    kParameterT4Wave,
    kParameterT4Tone,
    kParameterT4Bend,
    kParameterT4Decay,
    kParameterT4Click,
    kParameterT4Noise,
    kParameterT4Filter,
    kParameterT4Level,

    // Master
    kParameterVolume,

    kParameterCount
};

#endif // DISTRHO_PLUGIN_INFO_H_INCLUDED
