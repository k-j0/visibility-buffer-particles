#pragma once

#include "Utils.h"
#include <vulkan/vulkan.hpp>
#include <vector>
#include <array>
#include <string>
#include <filesystem>
#include "Vertex.h"
#include "RenderPass.h"
#include "Bindable.h"

#define ENTRYPOINT_COMPUTE_SHADER_FUNCTION "main" // assuming all compute shaders will start from main() here

/// Wrapper to a VkPipeline used for Compute op
class ComputePipeline : public Bindable {
	
	VkPipeline pipeline;// Vulkan compute pipeline handle
	VkDevice* logicalDevice;


public:

	/// Builds a compute pipeline from the specified Compute shader
	ComputePipeline(const std::string& computeShaderFile, const VkPipelineLayout& pipelineLayout, VkDevice* logicalDevice);

	/// Cleans up Vulkan pipeline resources
	virtual ~ComputePipeline();

	/// Binds the compute pipeline to the command buffer at recording time
	void cmdBind(const VkCommandBuffer& cmdBuffer, int index) const override;

};// class ComputePipeline
