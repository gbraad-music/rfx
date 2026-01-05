#include "plugin.hpp"

Plugin* pluginInstance;

void init(Plugin* p) {
	pluginInstance = p;

	// Add MOD player modules to this plugin
	p->addModel(modelRM_Deck);
}
