#pragma once

#define GLFW_INCLUDE_NONE
#define GLFW_INCLUDE_VULKAN
#define GLM_FORCE_RADIANS
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.hpp>
#include <string>
#include <set>
#include "QueueFamilyIndices.h"



#define WINDOW_WIDTH 1024
#define WINDOW_HEIGHT 768


#define PICK_PHYSICAL_GPU // undef to use integrated chip instead of physical chip




/// Allows a check on a VkResult variable returned by a vkXxx function call
static inline void check_vk_result(VkResult r) {
	if (r == 0) return;
	printf("VkResult %d\n", r);
	if (r < 0) abort(); // abort upon error
}




/// Container struct to find and keep track of a Vulkan Instance, a Physical Device (GPU) and a Logical Device
struct VulkanDevices {

	VulkanDevices(const std::string& windowName, GLFWframebuffersizefun onFramebufferResized, void* glfwUserPointer, bool enableValidationLayers, const std::vector<const char*>& validationLayers, const std::vector<const char*>& deviceExtensions);
	~VulkanDevices();


	/// Returns the current size of the GLFW window
	inline VkExtent2D getWindowExtent() const {
		int width, height;
		glfwGetFramebufferSize(window, &width, &height);
		return { (uint32_t)width, (uint32_t)height };
	}

	/// Getters
	inline GLFWwindow* getWindow() const { return window; }
	inline const VkSurfaceKHR& getSurface() const { return surface; }
	inline const VkInstance& getInstance() const { return instance; }
	inline const VkPhysicalDevice& getPhysicalDevice() const { return physicalDevice; }
	inline VkDevice* getLogicalDevice() { return &logicalDevice; }
	inline uint32_t getGraphicsQueueFamily() const { return queueFamilies.graphicsFamily.value(); }
	inline uint32_t getPresentQeueuFamily() const { return queueFamilies.presentFamily.value(); }
	inline uint32_t getComputeQueueFamily() const { return queueFamilies.computeFamily.value(); }
	inline const VkQueue& getGraphicsQueue() const { return graphicsQueue; }
	inline const VkQueue& getPresentQueue() const { return presentQueue; }
	inline const VkQueue& getComputeQueue() const { return computeQueue; }

private:

	/// GLFW window initializer
	void createWindow(const std::string& windowName, GLFWframebuffersizefun onFramebufferResized, void* glfwUserPointer);

	/// Vulkan Instance initialization
	void createVulkanInstance(bool enableValidationLayers, const std::vector<const char*>& validationLayers);
	bool checkValidationLayerSupport(const std::vector<const char*>& validationLayers);

	/// Physical device initialization
	void pickPhysicalDevice(const std::vector<const char*>& deviceExtensions);
	bool checkPhysicalDevice(const VkPhysicalDevice& device, const std::vector<const char*>& deviceExtensions);
	bool checkDeviceExtensionSupport(const VkPhysicalDevice& device, const std::vector<const char*>& deviceExtensions);

	/// Logical device initialization
	void createLogicalDevice(bool enableValidationLayers, const std::vector<const char*>& validationLayers, const std::vector<const char*>& deviceExtensions);



	/// GLFW window instance & KHR surface to render onto
	GLFWwindow* window = NULL;
	VkSurfaceKHR surface = VK_NULL_HANDLE;

	/// Vulkan instance, physical (GPU) and logical device
	VkInstance instance = VK_NULL_HANDLE;
	VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
	VkDevice logicalDevice = VK_NULL_HANDLE;

	/// Queue family indices
	QueueFamilyIndices queueFamilies;

	/// Queues
	VkQueue graphicsQueue = VK_NULL_HANDLE;
	VkQueue presentQueue = VK_NULL_HANDLE;
	VkQueue computeQueue = VK_NULL_HANDLE;


};// struct VulkanDevices


/// Used instead of VulkanDevices raw pointer; adds syntactic sugar to access logical device through DevicesPtr::operator(). All other functions of VulkanDevices can be accessed as usual through operator->.
struct DevicesPtr {
	VulkanDevices* devices;
	/// Simplified version of VulkanDevices::getLogicalDevice() as it is very common
	inline VkDevice* operator()() const { return devices->getLogicalDevice(); }
	/// Allow access to other core functions through normal pointer logic.
	inline VulkanDevices* operator->() const { return devices; }
};// struct DevicesPtr

