#pragma once

#include "Utils.h"
#include <vulkan/vulkan.hpp>
#include "VulkanDevices.h"

/// Represents the swapchain with its set of framebuffers and images. Does not allow resizing, so a new Swapchain object needs to be created when the window is resized.
struct Swapchain {

	/// Helper struct for holding onto KHR swapchain capability information
	struct SupportDetails {
		VkSurfaceCapabilitiesKHR capabilities;
		std::vector<VkSurfaceFormatKHR> formats;
		std::vector<VkPresentModeKHR> presentModes;

		inline SupportDetails() {}
		inline SupportDetails(const VkPhysicalDevice& physicalDevice, const VkSurfaceKHR& surface) { (*this)(physicalDevice, surface); }

		void operator()(const VkPhysicalDevice& physicalDevice, const VkSurfaceKHR& surface);
	};// struct SupportDetails



	/// Constructor & destructor, must be both called anytime the swapchain needs resized
	Swapchain(DevicesPtr devices);
	~Swapchain();

	/// Must be called in init() and swapchainCreate() to setup framebuffer attachments and render pass
	void createFramebuffers(std::vector<VkImageView>& attachments, const VkRenderPass& renderPass);


	/// Getters
	inline const VkSwapchainKHR& getSwapchain() { return swapchain; }
	inline const VkExtent2D& getExtent() { return extent; }
	inline const VkFormat& getFormat() { return format; }
	inline uint32_t getSize() { return (uint32_t)images.size(); }
	inline const VkFramebuffer& getFramebuffer(int index) { return framebuffers[index]; }

private:

	/// Helper functions used in swapchain creation logic
	VkSurfaceFormatKHR pickSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
	VkPresentModeKHR pickSwapPresentMode(const std::vector<VkPresentModeKHR>& availableModes);
	VkExtent2D pickSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities, const VkExtent2D& windowExtent);
	void createImageViews();

	DevicesPtr devices;
	VkSwapchainKHR swapchain = VK_NULL_HANDLE;
	VkFormat format;
	VkExtent2D extent;
	std::vector<VkImage> images;
	std::vector<VkImageView> imageViews;
	std::vector<VkFramebuffer> framebuffers;

};// struct Swapchain
