#pragma once

#include "Utils.h"
#include <vulkan/vulkan.hpp>
#include <optional>

/// Helper struct to hold on to required family queue indices
struct QueueFamilyIndices {

	/// The queue families we're interested in
	std::optional<uint32_t> graphicsFamily;
	std::optional<uint32_t> presentFamily;
	std::optional<uint32_t> computeFamily;

	/// Returns true if all queue families have been filled in
	inline bool isComplete() { return graphicsFamily.has_value() && presentFamily.has_value() && computeFamily.has_value(); }

	/// Default (empty) constructor
	inline QueueFamilyIndices() {};

	/// Finds the family indices for the specified graphics card
	inline QueueFamilyIndices(const VkPhysicalDevice& physicalDevice, const VkSurfaceKHR& surface, bool verbose = false) {

		/// Find queue families present on the device
		uint32_t queueFamilyCount = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, NULL);
		std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
		vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, queueFamilies.data());

		/// Go through the families to find the ones we're interested in
		if (verbose) printf("Queues available:\n");
		for (int i = 0; i < queueFamilies.size(); ++i) {

			/// Print info about the queue
			if (verbose) {
				printf("\tQueue Family %d: %d queue(s) ( ", i, queueFamilies[i].queueCount);
				if (queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) printf("Graphics ");
				if (queueFamilies[i].queueFlags & VK_QUEUE_COMPUTE_BIT) printf("Compute ");
				if (queueFamilies[i].queueFlags & VK_QUEUE_TRANSFER_BIT) printf("Transfer ");
				printf(")\n");
			}

			//Check for graphics queue
			if (queueFamilies[i].queueCount > 0 && queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
				graphicsFamily = i;
			}

			//Check for surface presentation queue
			VkBool32 presentSupport = false;
			if (vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, i, surface, &presentSupport) == VK_SUCCESS) {
				if (queueFamilies[i].queueCount > 0 && presentSupport) {
					presentFamily = i;
				}
			}

			//Check for compute queue
			if (queueFamilies[i].queueCount > 0 && queueFamilies[i].queueFlags & VK_QUEUE_COMPUTE_BIT && !(queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)) {
				computeFamily = i;
			}

			if (isComplete()) break;//found all queues we need
		}

		/// Still haven't found compute queue? Might have to use a Graphics/Compute queue instead then
		if (graphicsFamily.has_value() && !computeFamily.has_value()) {
			for (int i = 0; i < queueFamilies.size(); ++i) {
				if (queueFamilies[i].queueCount > 0 && queueFamilies[i].queueFlags & VK_QUEUE_COMPUTE_BIT) {
					computeFamily = i;
					break;
				}
			}
		}

		/// Debug output if verbose flag set
		if (verbose) {
			if (isComplete()) {
				if (graphicsFamily.value() == presentFamily.value()) {
					printf("Graphics queue family and surface presentation queue family are the same on this device.\n");
				} else {
					printf("[Warning] Graphics queue family and surface presentation queue family are different on this device\n");//not having both be the same queue could impact performance
				}
				if (graphicsFamily.value() == computeFamily.value()) {
					printf("[Warning] Graphics queue family and compute queue family are the same on this device.\n");//having both be the same queue could impact performance
				}
				printf("- Graphics queue family: %d queues\n", queueFamilies[graphicsFamily.value()].queueCount);
				printf("- Compute queue family: %d queues\n", queueFamilies[computeFamily.value()].queueCount);
				printf("- Presentation queue family: %d queues\n", queueFamilies[presentFamily.value()].queueCount);
			} else {
				printf("[Error] Could not find queue families on this device\n");
			}
		}

	}

};// struct QueueFamilyIndices
