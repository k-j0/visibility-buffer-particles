#pragma once

#include <vulkan/vulkan.h>
#include "Utils.h"
#include "Mesh.h"
#include "MeshFactory.h"
#include "GraphicsPipeline.h"
#include "Descriptor.h"
#include "RenderPass.h"
#include "MatrixBuffer.h"
#include "LightBuffer.h"
#include "VulkanAppBase.h"
#include "UIOverlay.h"
#include "ComputePipeline.h"

/// Base class for a single scene, which should contain at least one render pass, several meshes, graphics pipelines, descriptors, textures etc.
class Scene {

protected:
	/// Variables accessible from the scene constructors, useful for creation of all resources. These are accessible through the VulkanAppBase ponter, but their inclusion here makes it easier to access.
	DevicesPtr devices;
	const VkCommandPool* commandPool;
	const VkDescriptorPool* descriptorPool;
	VulkanAppBase* vulkanApp;// pointer to the vulkan app base, which can be used to access other variables needed to create objects.

public:

	/// Creates the scene object.
	inline Scene(VulkanAppBase* vulkanApp) : vulkanApp(vulkanApp), devices(vulkanApp->devices), commandPool(vulkanApp->getCommandPool()), descriptorPool(vulkanApp->getDescriptorPool()) {}
	inline virtual ~Scene() {}// expected to perform cleanup of all resources used by the scene.

	/// Updates the scene; called each frame
	virtual void Update(uint32_t imageIndex, float dt, float time) = 0;

	/// Updates the graphical user interface
	/// Returns false normally - returns true if the current scene must be rebuilt
	virtual bool UI() = 0;

	/// Used to update a command buffer with the scene data; returns the last render pass, which must not have been ended yet (for UI overlay)
	virtual RenderPass* cmdBind(const VkCommandBuffer& cmdBuffer, int index) = 0;

	/// Used to update a compute command buffer with the scene data
	virtual void cmdBindCompute(const VkCommandBuffer& cmdBuffer) = 0;

	/// Returns the (last) render pass used by this scene
	virtual RenderPass* getRenderPass() = 0;

};// class Scene
