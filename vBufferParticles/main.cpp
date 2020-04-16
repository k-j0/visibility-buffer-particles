
#include "VulkanApplication.h"

#include <iostream>
#include <stdexcept>
#include <vector>
#include <string>
#include "StaticSettings.h"
#include "Utils.h"
#include "Particles.h"

//#define CATCH_EXCEPTIONS // commented out to not catch any thrown exceptions in main()

int main(int argc, char ** argv) {

	// Apply command-line arguments to create runtime constant settings
	if (argc > 0) {
		RuntimeConstantSettings settings;
		std::cout << "Applying command-line arguments:" << std::endl;
		for (int i = 0; i < argc; ++i) {
			std::string arg = argv[i];
			if (arg[0] == '-') {
				std::vector<std::string> splitArg = U::splitStr(":", arg.substr(1));
				if (splitArg.size() == 2) {
					std::cout << "\tSetting " << splitArg[0] << " = " << splitArg[1] << std::endl;
					std::string sn = splitArg[0];// setting name
					std::string sv = splitArg[1];// setting value (as string)

					// determine which argument it corresponds to
					if (sn == "ui") {
						settings.noUI = sv == "0";
					} else if (sn == "width") {
						settings.windowWidth = std::stoi(sv);
					} else if (sn == "height") {
						settings.windowHeight = std::stoi(sv);
					} else if (sn == "shadercomp") {
						settings.recompileShaders = sv == "1";
					} else if (sn == "renderer") {
						settings.renderer = sv == "fwd" ? RuntimeConstantSettings::Renderer::Fwd : sv == "g3" ? RuntimeConstantSettings::Renderer::G3 : sv == "g6" ? RuntimeConstantSettings::Renderer::G6 : RuntimeConstantSettings::Renderer::V;
					} else if (sn == "pmode") {
						settings.pMode = sv == "vege" ? RuntimeConstantSettings::ParticleMode::VeGe : sv == "ge" ? RuntimeConstantSettings::ParticleMode::Ge : sv == "co" ? RuntimeConstantSettings::ParticleMode::Co : RuntimeConstantSettings::ParticleMode::Ve;
					} else if (sn == "pspread") {
						settings.pSpread = std::stof(sv);
					} else if (sn == "psize") {
						settings.pHalfSize = std::stof(sv);
					} else if (sn == "pcount") {
						settings.pCount = std::stoi(sv);
					} else if (sn == "pcomplexity") {
						settings.pComplexity = std::stoi(sv);
					} else if (sn == "freeze") {
						settings.freezeTime = sv == "1";
					} else if(sn == "cutout") {
						ParticleSystem::setParticlesCutout(sv == "1");
					} else {
						std::cout << "Unknown setting: " << sn << std::endl;
					}

				}
			} // else- the argument doesn't start with a dash, ignore it.
		}
		std::cout << std::endl;
		StaticSettings::createInstance(settings);// apply rc settings
		ParticleSystem::setParticlesComplexity(RC_SETTINGS->pComplexity);// apply new particle complexity before anything else.
	} // command-line arguments

#ifdef CATCH_EXCEPTIONS
	try {
		// Run application
		VulkanApplication app;
		app.run();
	} catch (std::exception e) {
		// Catch any runtime exceptions
		std::cerr << e.what() << std::endl;
		system("PAUSE"); // pause to allow checking what exception occured.
		return 1;
	}
#else
	// Run application
	VulkanApplication app;
	app.run();
#endif

	// clean up any runtime constant settings used.
	if (RC_SETTINGS)
		delete StaticSettings::getInstance();

	return 0;
}
