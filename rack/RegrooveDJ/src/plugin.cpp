#include "plugin.hpp"

Plugin* pluginInstance;

void init(Plugin* p) {
	pluginInstance = p;

	// Add DJ modules to this plugin
	p->addModel(modelRDJ_Fader);
	p->addModel(modelRDJ_XFader);
}
