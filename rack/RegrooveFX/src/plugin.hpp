#pragma once
#include <rack.hpp>

using namespace rack;

// Declare the Plugin, defined in plugin.cpp
extern Plugin* pluginInstance;

// Declare each Model, defined in each module source file
extern Model* modelRFX_EQ;
extern Model* modelRFX_Distortion;
extern Model* modelRFX_Compressor;
extern Model* modelRFX_Filter;
extern Model* modelRFX_Delay;
extern Model* modelRFX_Reverb;
extern Model* modelRFX_StereoWiden;
