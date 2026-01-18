/*
 * RGSlicer - Slicing Sampler Plugin
 * VST3/LV2/Standalone Plugin
 * DISTRHO Plugin Framework
 */

#ifndef DISTRHO_PLUGIN_INFO_H
#define DISTRHO_PLUGIN_INFO_H

#define DISTRHO_PLUGIN_BRAND           "Regroove"
#define DISTRHO_PLUGIN_NAME            "RGSlicer"
#define DISTRHO_PLUGIN_URI             "https://regroove.org/plugins/rgslicer"
#define DISTRHO_PLUGIN_CLAP_ID         "org.regroove.rgslicer"

#define DISTRHO_PLUGIN_HAS_UI          1
#define DISTRHO_UI_USE_NANOVG          0
#define DISTRHO_UI_FILE_BROWSER        1
#define DISTRHO_UI_USER_RESIZABLE      0
#define DISTRHO_UI_DEFAULT_WIDTH       600
#define DISTRHO_UI_DEFAULT_HEIGHT      500
#define DISTRHO_PLUGIN_IS_RT_SAFE      1
#define DISTRHO_PLUGIN_IS_SYNTH        1
#define DISTRHO_PLUGIN_NUM_INPUTS      0
#define DISTRHO_PLUGIN_NUM_OUTPUTS     2
#define DISTRHO_PLUGIN_WANT_PROGRAMS   1
#define DISTRHO_PLUGIN_WANT_STATE      1
#define DISTRHO_PLUGIN_WANT_FULL_STATE 1
#define DISTRHO_PLUGIN_WANT_MIDI_INPUT 1
#define DISTRHO_PLUGIN_WANT_TIMEPOS    0

enum Parameters {
    // Global Parameters
    PARAM_MASTER_VOLUME = 0,
    PARAM_MASTER_PITCH,
    PARAM_MASTER_TIME,

    // Per-Slice Parameters (Slice 0 only for now)
    PARAM_SLICE0_PITCH,
    PARAM_SLICE0_TIME,
    PARAM_SLICE0_VOLUME,
    PARAM_SLICE0_PAN,
    PARAM_SLICE0_LOOP,
    PARAM_SLICE0_ONE_SHOT,

    // Random Sequencer
    PARAM_BPM,

    // Output parameters (read-only, for UI visualization)
    PARAM_PLAYBACK_POS,    // Active voice playback position (0.0-1.0)
    PARAM_PLAYING_SLICE,   // Currently playing slice index (-1 = none, -2 = full sample)

    PARAM_COUNT
};

#endif // DISTRHO_PLUGIN_INFO_H
