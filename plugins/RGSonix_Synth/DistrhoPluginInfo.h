/*
 * RGSonix - Regroove Aegis Sonix Style Wavetable Synthesizer
 * DISTRHO Plugin Framework (DPF)
 */

#ifndef DISTRHO_PLUGIN_INFO_H_INCLUDED
#define DISTRHO_PLUGIN_INFO_H_INCLUDED

// Display strings
#define RGSONIX_DISPLAY_NAME "RGSonix - Regroove Wavetable Synthesizer"
#define RGSONIX_DESCRIPTION "Aegis Sonix Style Single-Cycle Wavetable Synth with Drawable Waveforms"
#define RGSONIX_WINDOW_TITLE "RGSonix Synthesizer"

#define DISTRHO_PLUGIN_BRAND "Regroove"
#define DISTRHO_PLUGIN_NAME  RGSONIX_DISPLAY_NAME
#define DISTRHO_PLUGIN_URI   "https://music.gbraad.nl/regrooved/rgsonix"

#define DISTRHO_PLUGIN_HAS_UI        1
#define DISTRHO_PLUGIN_IS_RT_SAFE    1
#define DISTRHO_PLUGIN_IS_SYNTH      1
#define DISTRHO_PLUGIN_NUM_INPUTS    0
#define DISTRHO_PLUGIN_NUM_OUTPUTS   2
#define DISTRHO_PLUGIN_WANT_PROGRAMS 0
#define DISTRHO_PLUGIN_WANT_STATE    1  // For waveform data
#define DISTRHO_PLUGIN_WANT_TIMEPOS  0
#define DISTRHO_PLUGIN_WANT_MIDI_INPUT 1

// UI
#define DISTRHO_UI_USE_NANOVG       0
#define DISTRHO_UI_USER_RESIZABLE   1
#define DISTRHO_UI_DEFAULT_WIDTH    800
#define DISTRHO_UI_DEFAULT_HEIGHT   600

// Wavetable constants
#define WAVETABLE_SIZE 128

// Parameters
enum Parameters {
    // Envelope
    kParameterAttack = 0,
    kParameterDecay,
    kParameterSustain,
    kParameterRelease,

    // Pitch
    kParameterPitchBend,
    kParameterFineTune,
    kParameterCoarseTune,

    // Master
    kParameterVolume,

    kParameterCount
};

#endif // DISTRHO_PLUGIN_INFO_H_INCLUDED
