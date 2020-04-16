#pragma once

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_vulkan.h"
#include "Bindable.h"
#include "VulkanDevices.h"
#include "RenderPass.h"
#include <functional>


/// Handles the overlay UI settings through ImGui
class UIOverlay : public Bindable {

	DevicesPtr devices;
	
	// Flags used for UI
	bool initialized = false;
	int frames = 0;// how many frames passed since the last call to setup()

	/// Function that will be called to update the UI, which should contain all ImGui calls
	std::function<void()> uiCallback;

public:

	/// Initializes ImGui
	UIOverlay(DevicesPtr devices, std::function<void()> uiCallback);

	/// Sets up ImGui for use with Vk app
	void setupGUI(const VkDescriptorPool& descriptorPool, const VkCommandPool& commandPool, uint16_t swapchainSize, const RenderPass* renderPass);

	/// Cleans up ImGui resources
	~UIOverlay();

	/// Updates the overlay interface; returns true upon any changes detected in the UI.
	bool updateGUI();

	/// Binds the ui overlay to the command buffers for rendering
	void cmdBind(const VkCommandBuffer& cmdBuffer, int index) const override;


};// class UIOVerlay
