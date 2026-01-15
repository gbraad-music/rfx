/*
 * RGDeckPlayer - Regroove Unified Tracker Player
 * DISTRHO Plugin Framework (DPF)
 */

#ifndef DISTRHO_PLUGIN_INFO_H_INCLUDED
#define DISTRHO_PLUGIN_INFO_H_INCLUDED

// Display strings
#define RGDECKPLAYER_DISPLAY_NAME "RGDeckPlayer - Unified Tracker Player"
#define RGDECKPLAYER_DESCRIPTION "MOD/MED/AHX/SID File Player"
#define RGDECKPLAYER_WINDOW_TITLE "RGDeckPlayer"

#define DISTRHO_PLUGIN_BRAND "Regroove"
#define DISTRHO_PLUGIN_NAME  RGDECKPLAYER_DISPLAY_NAME
#define DISTRHO_PLUGIN_URI   "https://music.gbraad.nl/regrooved/rgdeckplayer"

#define DISTRHO_PLUGIN_HAS_UI        1
#define DISTRHO_PLUGIN_IS_RT_SAFE    1
#define DISTRHO_PLUGIN_IS_SYNTH      1
#define DISTRHO_PLUGIN_NUM_INPUTS    0
#define DISTRHO_PLUGIN_NUM_OUTPUTS   10  // 4 stereo channel pairs (8) + 1 stereo mix (2)
#define DISTRHO_PLUGIN_WANT_PROGRAMS 0
#define DISTRHO_PLUGIN_WANT_STATE    1  // For file path
#define DISTRHO_PLUGIN_WANT_FULL_STATE 1  // For getState
#define DISTRHO_PLUGIN_WANT_TIMEPOS  0
#define DISTRHO_PLUGIN_WANT_MIDI_INPUT 1

// UI
#define DISTRHO_UI_FILE_BROWSER      1  // Enable file browser for loading tracker files

// UI
#define DISTRHO_UI_USE_NANOVG       0
#define DISTRHO_UI_USER_RESIZABLE   1
#define DISTRHO_UI_DEFAULT_WIDTH    200
#define DISTRHO_UI_DEFAULT_HEIGHT   400

// Parameters
enum Parameters {
    // Playback controls
    kParameterPlay = 0,
    kParameterLoopPattern,
    kParameterPrevPattern,
    kParameterNextPattern,
    kParameterLoopStart,
    kParameterLoopEnd,
    kParameterBPM,
    kParameterOutputMode,  // 0 = Stereo Mix, 1 = Multi-channel
    kParameterMasterMute,  // Master output mute (priming)

    // Position display (read-only outputs)
    kParameterCurrentOrder,
    kParameterCurrentRow,

    // Channel 1
    kParameterCh1Mute,
    kParameterCh1Volume,
    kParameterCh1Pan,

    // Channel 2
    kParameterCh2Mute,
    kParameterCh2Volume,
    kParameterCh2Pan,

    // Channel 3
    kParameterCh3Mute,
    kParameterCh3Volume,
    kParameterCh3Pan,

    // Channel 4
    kParameterCh4Mute,
    kParameterCh4Volume,
    kParameterCh4Pan,

    kParameterCount
};

#endif // DISTRHO_PLUGIN_INFO_H_INCLUDED
