/*
 * RGAHX Synth - AHX Instrument Synthesizer Plugin
 * Distrho Plugin Information
 */

#ifndef DISTRHO_PLUGIN_INFO_H
#define DISTRHO_PLUGIN_INFO_H

#define DISTRHO_PLUGIN_BRAND "Regroove"
#define DISTRHO_PLUGIN_NAME  "RGAHX Synth"
#define DISTRHO_PLUGIN_URI   "https://music.gbraad.nl/regrooved/rgahx-synth"

#define DISTRHO_PLUGIN_HAS_UI        0
#define DISTRHO_PLUGIN_IS_RT_SAFE    1
#define DISTRHO_PLUGIN_IS_SYNTH      1
#define DISTRHO_PLUGIN_NUM_INPUTS    0
#define DISTRHO_PLUGIN_NUM_OUTPUTS   2
#define DISTRHO_PLUGIN_WANT_PROGRAMS 0
#define DISTRHO_PLUGIN_WANT_STATE    0
#define DISTRHO_PLUGIN_WANT_TIMEPOS  0

#define RGAHX_DISPLAY_NAME "RGAHX Synth"
#define RGAHX_DESCRIPTION  "AHX/HVL Instrument Synthesizer"

// Parameter groups for better organization
enum ParameterGroup {
    GROUP_OSCILLATOR,
    GROUP_ENVELOPE,
    GROUP_FILTER,
    GROUP_PWM,
    GROUP_VIBRATO,
    GROUP_RELEASE,
    GROUP_MASTER
};

// All controllable parameters
enum Parameters {
    // Oscillator (Group 0)
    kParameterWaveform = 0,
    kParameterWaveLength,
    kParameterOscVolume,

    // Envelope (Group 1)
    kParameterAttackFrames,
    kParameterAttackVolume,
    kParameterDecayFrames,
    kParameterDecayVolume,
    kParameterSustainFrames,
    kParameterReleaseFrames,
    kParameterReleaseVolume,

    // Filter (Group 2)
    kParameterFilterLower,
    kParameterFilterUpper,
    kParameterFilterSpeed,
    kParameterFilterEnable,

    // PWM / Square Modulation (Group 3)
    kParameterSquareLower,
    kParameterSquareUpper,
    kParameterSquareSpeed,
    kParameterSquareEnable,

    // Vibrato (Group 4)
    kParameterVibratoDelay,
    kParameterVibratoDepth,
    kParameterVibratoSpeed,

    // Release (Group 5)
    kParameterHardCutRelease,
    kParameterHardCutFrames,

    // Master (Group 6)
    kParameterMasterVolume,

    kParameterCount
};

#endif // DISTRHO_PLUGIN_INFO_H
