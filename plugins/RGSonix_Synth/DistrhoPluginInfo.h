/*
 * RGSonix - Regroove Aegis Sonix Style Wavetable Synthesizer
 * DISTRHO Plugin Framework (DPF)
 */

#ifndef DISTRHO_PLUGIN_INFO_H_INCLUDED
#define DISTRHO_PLUGIN_INFO_H_INCLUDED

// Display strings
#define RGSONIX_DISPLAY_NAME "RGSonix - Regroove Wavetable Synthesizer"
#define RGSONIX_DESCRIPTION "Aegis Sonix Style Multi-Oscillator Wavetable Synth with Drawable Waveforms"
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
#define DISTRHO_UI_DEFAULT_WIDTH    1200
#define DISTRHO_UI_DEFAULT_HEIGHT   700

// Wavetable constants
#define WAVETABLE_SIZE 128
#define NUM_WAVEFORMS 8
#define NUM_OSCILLATORS 4

// Parameters
enum Parameters {
    // Oscillator 1
    kParameterOsc1Wave = 0,
    kParameterOsc1Level,
    kParameterOsc1Detune,
    kParameterOsc1Phase,
    kParameterOsc1AmpAttack,
    kParameterOsc1AmpDecay,
    kParameterOsc1AmpSustain,
    kParameterOsc1AmpRelease,
    kParameterOsc1PitchAttack,
    kParameterOsc1PitchDecay,
    kParameterOsc1PitchDepth,

    // Oscillator 2
    kParameterOsc2Wave,
    kParameterOsc2Level,
    kParameterOsc2Detune,
    kParameterOsc2Phase,
    kParameterOsc2AmpAttack,
    kParameterOsc2AmpDecay,
    kParameterOsc2AmpSustain,
    kParameterOsc2AmpRelease,
    kParameterOsc2PitchAttack,
    kParameterOsc2PitchDecay,
    kParameterOsc2PitchDepth,

    // Oscillator 3
    kParameterOsc3Wave,
    kParameterOsc3Level,
    kParameterOsc3Detune,
    kParameterOsc3Phase,
    kParameterOsc3AmpAttack,
    kParameterOsc3AmpDecay,
    kParameterOsc3AmpSustain,
    kParameterOsc3AmpRelease,
    kParameterOsc3PitchAttack,
    kParameterOsc3PitchDecay,
    kParameterOsc3PitchDepth,

    // Oscillator 4
    kParameterOsc4Wave,
    kParameterOsc4Level,
    kParameterOsc4Detune,
    kParameterOsc4Phase,
    kParameterOsc4AmpAttack,
    kParameterOsc4AmpDecay,
    kParameterOsc4AmpSustain,
    kParameterOsc4AmpRelease,
    kParameterOsc4PitchAttack,
    kParameterOsc4PitchDecay,
    kParameterOsc4PitchDepth,

    // Global Pitch
    kParameterFineTune,
    kParameterCoarseTune,

    // Velocity Sensitivity
    kParameterVelToAmp,
    kParameterVelToPitch,
    kParameterVelToAttack,
    kParameterVelToWave,

    // Master
    kParameterVolume,

    kParameterCount
};

#endif // DISTRHO_PLUGIN_INFO_H_INCLUDED
