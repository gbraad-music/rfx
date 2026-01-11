#ifndef DISTRHO_PLUGIN_INFO_H_INCLUDED
#define DISTRHO_PLUGIN_INFO_H_INCLUDED

#define DISTRHO_PLUGIN_BRAND "Regroove"
#define DISTRHO_PLUGIN_NAME  "RG SFZ Player"
#define DISTRHO_PLUGIN_URI   "https://music.gbraad.nl/regrooved/rgsfz"

#define DISTRHO_PLUGIN_HAS_UI        1
#define DISTRHO_PLUGIN_IS_RT_SAFE    1
#define DISTRHO_PLUGIN_NUM_INPUTS    0
#define DISTRHO_PLUGIN_NUM_OUTPUTS   2
#define DISTRHO_PLUGIN_WANT_TIMEPOS  0
#define DISTRHO_PLUGIN_WANT_PROGRAMS 0
#define DISTRHO_PLUGIN_WANT_STATE    1
#define DISTRHO_PLUGIN_WANT_MIDI_INPUT  1
#define DISTRHO_PLUGIN_WANT_MIDI_OUTPUT 0

enum Parameters {
    kParameterVolume = 0,
    kParameterPan,
    kParameterAttack,
    kParameterDecay,
    kParameterCount
};

#define RGSFZ_DISPLAY_NAME "RG SFZ Player"
#define RGSFZ_DESCRIPTION "Simple SFZ sample player - drag and drop SFZ files"

#endif
