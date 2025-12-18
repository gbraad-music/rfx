#ifndef DISTRHO_PLUGIN_INFO_H_INCLUDED
#define DISTRHO_PLUGIN_INFO_H_INCLUDED

#define DISTRHO_PLUGIN_BRAND "Regroove"
#define DISTRHO_PLUGIN_NAME  "RFX Phaser"
#define DISTRHO_PLUGIN_URI   "https://github.com/gbraad/rfx/phaser"

#define DISTRHO_PLUGIN_HAS_UI        1
#define DISTRHO_PLUGIN_IS_RT_SAFE       1
#define DISTRHO_PLUGIN_NUM_INPUTS       2
#define DISTRHO_PLUGIN_NUM_OUTPUTS      2
#define DISTRHO_PLUGIN_WANT_PROGRAMS    0
#define DISTRHO_PLUGIN_WANT_STATE       1
#define DISTRHO_PLUGIN_WANT_FULL_STATE  1
#define DISTRHO_PLUGIN_WANT_TIMEPOS     0

#define DISTRHO_PLUGIN_LV2_CATEGORY "lv2:PhaserPlugin"
#define DISTRHO_PLUGIN_VST3_CATEGORIES "Fx|Modulation"

#define DISTRHO_UI_DEFAULT_WIDTH  200
#define DISTRHO_UI_DEFAULT_HEIGHT 300

enum Parameters {
    kParameterRate = 0,
    kParameterDepth,
    kParameterFeedback,
    kParameterCount
};

#endif
