#ifndef DISTRHO_PLUGIN_INFO_H_INCLUDED
#define DISTRHO_PLUGIN_INFO_H_INCLUDED

#define DISTRHO_PLUGIN_BRAND "Regroove"
#define DISTRHO_PLUGIN_NAME  "RFX Compressor"
#define DISTRHO_PLUGIN_URI   "https://github.com/gbraad/rfx/compressor"

#define DISTRHO_PLUGIN_HAS_UI        0
#define DISTRHO_PLUGIN_IS_RT_SAFE    1
#define DISTRHO_PLUGIN_NUM_INPUTS    2
#define DISTRHO_PLUGIN_NUM_OUTPUTS   2
#define DISTRHO_PLUGIN_WANT_PROGRAMS 0
#define DISTRHO_PLUGIN_WANT_STATE    0
#define DISTRHO_PLUGIN_WANT_TIMEPOS  0

#define DISTRHO_PLUGIN_LV2_CATEGORY "lv2:CompressorPlugin"
#define DISTRHO_PLUGIN_VST3_CATEGORIES "Fx|Dynamics"

enum Parameters {
    kParameterThreshold = 0,
    kParameterRatio,
    kParameterAttack,
    kParameterRelease,
    kParameterMakeup,
    kParameterCount
};

#endif
