#pragma once

#include "Utils.h"
#include <vulkan/vulkan.hpp>

/// A uniform buffer that can be sent to shader stage; templated to allow any kind of buffer to be sent.
template<typename UBO>
struct UniformBuffer {

	/// Create uniform buffer + buffer memory for each swapchain image.
	inline UniformBuffer(int swapchainSize, VkDevice* logicalDevice, VkPhysicalDevice physicalDevice,
				int usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,			// Default usage is as a Uniform Buffer, but this can be optionally modified to use as SSBO also
				int memoryProperties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,	// Default memory properties for use as a Uniform Buffer
				int size = 1	// Size multiplier for the UBO
			) {

		this->logicalDevice = logicalDevice;

		VkDeviceSize bufferSize = sizeof(UBO) * size;

		uniformBuffers.resize(swapchainSize);
		uniformBuffersMemory.resize(swapchainSize);

		for (size_t i = 0; i < swapchainSize; ++i) {
			U::createBuffer(bufferSize, (VkBufferUsageFlagBits)usage, (VkMemoryPropertyFlagBits)memoryProperties, uniformBuffers[i], uniformBuffersMemory[i], *logicalDevice, physicalDevice);
		}

	}

	/// Cleanup vk resources (buffers + memory)
	inline virtual ~UniformBuffer() {
		if (logicalDevice) {
			for (int i = 0; i < uniformBuffers.size(); ++i) {
				vkDestroyBuffer(*logicalDevice, uniformBuffers[i], NULL);
				vkFreeMemory(*logicalDevice, uniformBuffersMemory[i], NULL);
			}
		}
	}

	/// Access underlying vk res handles
	inline std::vector<VkBuffer>& getBuffers() { return uniformBuffers; }


	/// Inheriting classes should use this function to upload modified UBOs to shaders.
	inline void copyBuffer(uint32_t currentImage, const UBO& ubo) {
		void* data;
		vkMapMemory(*logicalDevice, uniformBuffersMemory[currentImage], 0, sizeof(ubo), 0, &data); {
			memcpy(data, &ubo, sizeof(ubo));
		} vkUnmapMemory(*logicalDevice, uniformBuffersMemory[currentImage]);
	}

	/// Call copyBuffer() for all images in swapchain.
	inline void copyAllBuffers(const UBO& ubo) {
		for (int i = 0; i < uniformBuffersMemory.size(); ++i)
			copyBuffer(i, ubo);
	}

private:
	VkDevice* logicalDevice = NULL;

	// uniform buffers (same size as swapchain images)
	std::vector<VkBuffer> uniformBuffers;
	std::vector<VkDeviceMemory> uniformBuffersMemory;

};// struct UniformBuffer

// Potentially a very confusing macro :) its intended use is to append at the end of the argument list for deriving class constructors; the macro will then expand to fill the
// required arguments for the base class, as well as initialize it. For ease of use, the constructor closing bracket is left out (so calling code will appear to have the
// closing bracket of the argument list).
// Potential pitfall: this has to be used at the end of the argument list, as any code before the closing bracket of the UniformBuffer constructor will be invalid
// (unless it fills the values of the optional parameters).
// This also means that code such as ```  inline ExtendedUniformBuffer(int foo, int bar, UNIFORM_BUFFER_CONSTRUCTOR), foo(foo) {}  ``` is perfectly valid despite looking like the
// usual colon has been substituted for a comma before foo(foo).
// Using this along with the optional arguments at the end of the function signature for UniformBuffer is not advised as such code will end up looking like this:
// ```  inline ExtendedUniformBuffer(int foo, UNIFORM_BUFFER_CONSTRUCTOR, VK_BUFFER_USAGE_x, VK_MEMORY_PROPERTY_y) {}  ``` which makes it look like the usage and memory property
// flags are part of the argument list; instead, use the full verbose version without this macro in this case.
#define UNIFORM_BUFFER_CONSTRUCTOR int swapchainSize, VkDevice* logicalDevice, VkPhysicalDevice physicalDevice) : UniformBuffer(swapchainSize, logicalDevice, physicalDevice
