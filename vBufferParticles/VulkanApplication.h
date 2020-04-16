#pragma once

#include "VulkanAppBase.h"

#include "Scene.h"
#include "UIOverlay.h"



#define FROZEN_TIME_SECONDS 0.f // time step at which simulation will be set to on all frames when Freeze Time is enabled


/// Takes care of switching between Scenes.
class VulkanApplication : public VulkanAppBase {
public:

	/// Cleanup.
	~VulkanApplication() override;

protected:

	/// Initializer
	void init() override;

	/// Frame update
	void frame(uint32_t currentImage, float dt, float time) override;

	/// UI update
	void ui();

	/// Called when creating swapchain
	void onSwapchainResize() override;

	/// Record a command buffer
	void recordCommandBuffer(VkCommandBuffer cmdBuffer, int index) override;

	/// Record the compute command buffer
	void recordComputeCommandBuffer(VkCommandBuffer cmdBuffer) override;

private:

	/// Create and cleanup resources bound to swapchain size
	void createSwapchainResources();
	void releaseSwapchainResources();

	Scene* currentScene = NULL;// the current scene, on the heap.
	uint8_t currentSceneIndex = 0;// used for switching between scenes

	UIOverlay* gui;// graphical user interface

	bool freezeTime = false;// should time be frozen?

	bool showGui = true;// toggle on key press to save on draw calls and updates when necessary.
	bool pressingToggleGui = false;// whether we are currently pressing the Toggle Gui key.

};// class VulkanApplication
