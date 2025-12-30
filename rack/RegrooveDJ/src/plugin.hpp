#pragma once
#include <rack.hpp>

using namespace rack;

// Declare the Plugin, defined in plugin.cpp
extern Plugin* pluginInstance;

// Declare each Model, defined in each module source file
extern Model* modelRDJ_Fader;
extern Model* modelRDJ_XFader;
extern Model* modelRDJ_Mixer;
