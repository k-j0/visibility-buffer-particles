#pragma once

#include <glm/glm.hpp>
#include <stdexcept>
#include <vector>
#include <optional>
#include <set>
#include <cstdint>
#include <algorithm>
#include <array>
#include <chrono>
#include "Utils.h"
#include "Input.h"
#include "Camera.h"
#include "Texture.h"
#include "RenderPass.h"
#include "GraphicsPipeline.h"
#include "Descriptor.h"
#include "Swapchain.h"
#include "QueueFamilyIndices.h"
#include "VulkanDevices.h"



#define PRINT_UPON_REPAINT //define this to log any repaints (ie recording the command buffers) to cout.


#define SUBMIT_COMPUTE // undef this to prevent application from submitting any compute work


//max amount of frames that can be prepared at once before being rendered
#define MAX_FRAMES_IN_FLIGHT 3



/// Only include validation layers in debug builds
#ifdef NDEBUG
#define ENABLE_VALIDATION_LAYERS false
#else
#define ENABLE_VALIDATION_LAYERS true
#endif

/// Only recompile shaders in debug builds
#ifndef NDEBUG
#define RECOMPILE_SHADERS true
#else
#define RECOMPILE_SHADERS false
#endif

/// Validation layers that will be turned on (only in Debug mode)
const std::vector<const char*> validationLayers = {
	"VK_LAYER_KHRONOS_validation"				// basic validation
	//,"VK_LAYER_GOOGLE_threading"				// check validity of multi-threaded APIs
	//,"VK_LAYER_LUNARG_parameter_validation"	// check validity of params in API calls
	//,"VK_LAYER_LUNARG_object_tracker"			// check validity of objects created & memory leaks
	//,"VK_LAYER_LUNARG_image"					// check texture & render target formats
	//,"VK_LAYER_LUNARG_core_validation"		// core validation
	//,"VK_LAYER_LUNARG_swapchain"				// check validitiy of swapchain
	//,"VK_LAYER_LUNARG_api_dump"				// print out all API calls
};

/// Device extensions that will be requested
const std::vector<const char*> deviceExtensions = {
	VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

/// Near and far plane distances (must match NEAR and FAR #defines in shaders)
#define NEAR 0.01f
#define FAR 100.f






/// The base class for a vulkan-based application
/// Features all low-level features needed for basic rendering
class VulkanAppBase {
public:

	/// Initialize and destroy application
	void run();

	/// Performs cleanup
	virtual ~VulkanAppBase();


private:

	/// Called upon startup
	void initialize();

	/// Called each frame
	void update(uint32_t imageIndex);
	void render();

	
	// Called upon GLFW window resized
	static void framebufferResizeCallback(GLFWwindow* window, int width, int height);


	/// Creation of resources separated into several functions.
	void recreateSwapchain();
	void cleanupSwapchain();
	void createCommandPools();
	void createDescriptorPool();
	void createCommandBuffers();
	void createSyncObjects();
	void cleanupSyncObjects();

	/// Record all command buffers (once they've been created)
	/// waitIdle: set to false ONLY when it is guaranteed that command buffers are not in use
	void recordCommandBuffers(bool waitIdle = true);

protected:

	/// Deletes all swapchain-tied resources and recreates them along with all command buffers
	inline void updateSwapchain() { framebufferResized = true; }


	///
	/// Overridable methods to implement functionality
	///

	virtual void init() = 0;

	/// Image index is the current image we should prepare; dt delta time; time the time since startup
	virtual void frame(uint32_t imageIndex, float dt, float time) = 0;

	/// Called when re-creating swapchain
	virtual void onSwapchainResize() = 0;

	/// Called to record a command buffer
	virtual void recordCommandBuffer(VkCommandBuffer cmdBuffer, int index) = 0;

	/// Called to record the compute command buffer
	virtual void recordComputeCommandBuffer(VkCommandBuffer cmdBuffer) = 0;


public:

	/// Call to trigger an immediate update of the command buffers.
	void Repaint();

	///
	/// Getters
	///

	inline VkCommandPool* getCommandPool() { return &commandPool; }
	inline VkCommandPool* getComputeCommandPool() { return &computeCommandPool; }
	inline const Camera& getCamera() { return camera; }
	inline const VkSampler& getSampler() { return sampler; }
	inline VkDescriptorPool* getDescriptorPool() { return &descriptorPool; }
	inline Swapchain* getSwapchain() { return swapchain; }
	inline Texture* getDepthBuffer() { return depthBuffer; }


	/// Vulkan instance & devices (includes GLFW window and KHR surface)
	DevicesPtr devices;

private:

	/// Swapchain
	Swapchain* swapchain;

	/// Descriptor and command pools
	VkDescriptorPool descriptorPool;
	VkCommandPool commandPool;
	VkCommandPool computeCommandPool;
	std::vector<VkCommandBuffer> commandBuffers;
	VkCommandBuffer computeCommandBuffer = VK_NULL_HANDLE;

	/// Sync objects
	std::vector<VkSemaphore> imageAvailableSemaphores;
	std::vector<VkSemaphore> renderFinishedSemaphores;
	std::vector<VkFence> inFlightFences;
	VkFence computeFence;

	/// Specific flags
	size_t currentFrame = 0;
	bool framebufferResized = false;

	/// Main camera
	Camera camera;

	/// Default texture sampler
	VkSampler sampler;

	/// Depth buffer texture
	Texture* depthBuffer;

};// class VulkanAppBase
