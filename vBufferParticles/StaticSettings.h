#pragma once

#include <stdexcept>

// All runtime constants that will be deduced from the startup arguments
struct RuntimeConstantSettings {

	bool noUI = false;// if true, will start with UI hidden
	unsigned int windowWidth = 1024;
	unsigned int windowHeight = 768;
	bool recompileShaders = true;
	enum class Renderer{ Fwd, G3, G6, V } renderer = Renderer::V;// which renderer to start in
	enum class ParticleMode{ Ve, Ge, Co, VeGe } pMode = ParticleMode::Ve;// which particle mode to use
	uint8_t pComplexity = 0;// particle fragment shader complexity
	float pSpread = 0.4f;// particle spread
	float pHalfSize = 0.03f;// particle half size
	unsigned int pCount = 1024 * 1024;// particle count
	bool freezeTime = false;

};// struct RuntimeConstantSettings

// Runtime constant settings, applied upon startup with args.
class StaticSettings {
	
private:

	static StaticSettings* instance;

	inline StaticSettings() {}// singleton - no instancing of this class.

	RuntimeConstantSettings settings;

public:
	static void createInstance(RuntimeConstantSettings settings);
	static inline StaticSettings* getInstance() { return instance; }
	static inline const RuntimeConstantSettings* getConstantSettings() { if (instance == NULL) return NULL; return &instance->settings; }

};// class StaticSettings

// abbreviation macro for runtime constants
#define RC_SETTINGS StaticSettings::getConstantSettings() // redefine this to NULL or false to completely disregard these runtime settings.
