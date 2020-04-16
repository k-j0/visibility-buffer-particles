#include "VulkanApplication.h"

#include "GBufferScene.h"
#include "GBuffer6Scene.h"
#include "ForwardRendererScene.h"
#include "VBufferScene.h"

#include <iostream>
#include "StaticSettings.h"


#define VBUFFER_SCENE_INDEX 0
#define GBUFFER_SCENE_INDEX 1
#define GBUFFER_6_SCENE_INDEX 2
#define FWD_SCENE_INDEX 3




void VulkanApplication::init() {

	if (RC_SETTINGS) showGui = !RC_SETTINGS->noUI;
	if (RC_SETTINGS) freezeTime = RC_SETTINGS->freezeTime;

	if (RC_SETTINGS) // select the right initial scene
		currentSceneIndex = RC_SETTINGS->renderer == RuntimeConstantSettings::Renderer::V ? VBUFFER_SCENE_INDEX :
							RC_SETTINGS->renderer == RuntimeConstantSettings::Renderer::G3 ? GBUFFER_SCENE_INDEX :
							RC_SETTINGS->renderer == RuntimeConstantSettings::Renderer::G6 ? GBUFFER_6_SCENE_INDEX :
							FWD_SCENE_INDEX;

	/// Initialize GUI with callbacks
	gui = new UIOverlay(devices, [this] {
		ui();
	});

	/// Create objects dependant on swapchain layout
	createSwapchainResources();

	/// Output controls documentation to the console.
	std::cout << std::endl << std::endl;
	std::cout << "\t\t\t\033[1;36m***  \033[4mVisibility Buffer Particles\033[24m  ***" << std::endl;
	std::cout << "\t\tControls:" << std::endl;
	std::cout << "\tW, A, S, D: Move camera horizontally" << std::endl;
	std::cout << "\tQ, E: Move camera vertically" << std::endl;
	std::cout << "\tSpacebar: Toggle FPS mode" << std::endl;
	std::cout << "\tMouse movement: Rotate camera (FPS mode only)" << std::endl;
	std::cout << "\033[0m" << std::endl << std::endl;

}

void VulkanApplication::createSwapchainResources() {

	/// Create next scene (nb: may still be the same)
	switch (currentSceneIndex) {
	case VBUFFER_SCENE_INDEX:
		currentScene = new VBufferScene(this); break;//deferred rendering with V-Buffer
	case GBUFFER_SCENE_INDEX:
		currentScene = new GBufferScene(this); break;//deferred rendering with 3-component G-Buffer
	case GBUFFER_6_SCENE_INDEX:
		currentScene = new GBuffer6Scene(this); break;//deferred rendering with 6-component G-Buffer
	case FWD_SCENE_INDEX:
		currentScene = new ForwardRendererScene(this); break;//forward
	}

	/// Setup GUI for current scene and swapchain
	gui->setupGUI(*getDescriptorPool(), *getCommandPool(), getSwapchain()->getSize(), currentScene->getRenderPass());

}




VulkanApplication::~VulkanApplication() {

	DELETE(gui);

	releaseSwapchainResources();

	/// Release resources independant from swapchain

}

void VulkanApplication::releaseSwapchainResources() {

	/// Release resources tied to swapchain size
	if (currentScene) DELETE(currentScene);
}




void VulkanApplication::frame(uint32_t currentImage, float dt, float time) {

	/// Update scene
	currentScene->Update(currentImage, dt, freezeTime ? FROZEN_TIME_SECONDS : time);

	/// Upon pressing T, toggle UI visibility
	if (Input::getInstance(devices->getWindow())->isKeyDown(GLFW_KEY_T)) {
		if (!pressingToggleGui) {
			pressingToggleGui = true;
			showGui = !showGui;
			Repaint();
			return;
		}
	} else pressingToggleGui = false;

	/// Upon changing the UI, update command buffers to reflect changes.
	if (showGui && gui->updateGUI())
		Repaint();

}

void VulkanApplication::ui() {

	if (!showGui) return;
	
	/// Controls display
	if (ImGui::CollapsingHeader("Controls")) {
		ImGui::Text("W, A, S, D: Move camera horizontally");
		ImGui::Text("Q, E: Move camera vertically");
		ImGui::Text("Space bar: Toggle FPS mode");
		ImGui::Text("Mouse movement: Rotate camera (FPS mode only)");
		ImGui::Separator();
	}// Controls display

	/// Whether we should freeze time
	ImGui::Checkbox("Freeze Time", &freezeTime);


	/// Drop-down list for scene displayed
	static const char* scenes[] = { "Visibility Buffer", "Geometry Buffer (3)", "Geometry Buffer (6)", "Forward Renderer" };
	if (ImGui::BeginCombo("##scenes", scenes[currentSceneIndex])) {
		for (int i = 0; i < IM_ARRAYSIZE(scenes); ++i) {
			bool isSelected = currentSceneIndex == i;
			if (ImGui::Selectable(scenes[i], isSelected) && !isSelected) {
				/// Go to next scene
				currentSceneIndex = i;
				updateSwapchain();
			}
			if (isSelected) {
				ImGui::SetItemDefaultFocus();
			}
		}
		ImGui::EndCombo();
	}// Scenes drop down

	ImGui::Separator();

	/// Scene-specific settings
	if (currentScene->UI()) {
		updateSwapchain();
	}

}




void VulkanApplication::onSwapchainResize() {
	/// Release resources from previous swapchain
	releaseSwapchainResources();
	/// Initialize resources
	createSwapchainResources();
}



/// Record scene-specific command buffers.
void VulkanApplication::recordCommandBuffer(VkCommandBuffer cmdBuffer, int index) {

	/// Record scene commands and get last render pass
	RenderPass* renderPass = currentScene->cmdBind(cmdBuffer, index);

	/// Record GUI commands
	if(showGui) gui->cmdBind(cmdBuffer, index);

	/// Finish up render pass
	renderPass->end(cmdBuffer);

}

/// Record scene-specific compute command buffers.
void VulkanApplication::recordComputeCommandBuffer(VkCommandBuffer cmdBuffer) {

	/// Record scene commands
	currentScene->cmdBindCompute(cmdBuffer);

}

