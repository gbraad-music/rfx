#include "plugin.hpp"

Plugin* pluginInstance;

void init(Plugin* p) {
	pluginInstance = p;

	// Add all effect modules to this plugin
	p->addModel(modelRFX_EQ);
	p->addModel(modelRFX_Distortion);
	p->addModel(modelRFX_Compressor);
	p->addModel(modelRFX_Filter);
	p->addModel(modelRFX_Delay);
	p->addModel(modelRFX_Reverb);
	p->addModel(modelRFX_StereoWiden);
}
