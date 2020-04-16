#include "StaticSettings.h"

StaticSettings* StaticSettings::instance = NULL;

void StaticSettings::createInstance(RuntimeConstantSettings settings) {
	if (instance != NULL) throw std::runtime_error("An instance of static settings was already created!");
	instance = new StaticSettings;
	instance->settings = settings;
}
