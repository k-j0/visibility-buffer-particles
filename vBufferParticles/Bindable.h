#pragma once

#include <vulkan/vulkan.hpp>

// Bindable interface: represents any object that can be bound at draw time
class Bindable {
public:
	/// Binds the resource to a command buffer
	virtual void cmdBind(const VkCommandBuffer& commandBuffer, int index) const = 0;
};