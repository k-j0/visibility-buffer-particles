#include "ComputePipeline.h"

ComputePipeline::ComputePipeline(const std::string& computeShaderFile, const VkPipelineLayout& pipelineLayout, VkDevice* logicalDevice) : logicalDevice(logicalDevice) {

	// Create shader stage

	auto compShaderCode = U::readFile("CompiledShaders/" + computeShaderFile + "_c.spv");
	VkShaderModule shaderModule = U::createShaderModule(compShaderCode, logicalDevice);

	VkPipelineShaderStageCreateInfo shaderStage = {};
	shaderStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	shaderStage.pNext = NULL;
	shaderStage.flags = 0;
	shaderStage.stage = VK_SHADER_STAGE_COMPUTE_BIT;
	shaderStage.module = shaderModule;
	shaderStage.pName = ENTRYPOINT_COMPUTE_SHADER_FUNCTION;
	shaderStage.pSpecializationInfo = NULL; // assume no specialization constants


	// Create pipeline

	VkComputePipelineCreateInfo info = {};
	info.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
	info.pNext = NULL;
	info.flags = 0;
	info.stage = shaderStage;
	info.layout = pipelineLayout;

	if (vkCreateComputePipelines(*logicalDevice, VK_NULL_HANDLE, 1, &info, NULL, &pipeline) != VK_SUCCESS)
		throw std::runtime_error("Failed to create compute pipeline");


	// Cleanup shader module
	vkDestroyShaderModule(*logicalDevice, shaderModule, NULL);

}

ComputePipeline::~ComputePipeline() {
	vkDestroyPipeline(*logicalDevice, pipeline, NULL);
}

void ComputePipeline::cmdBind(const VkCommandBuffer& cmdBuffer, int index) const {

	vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline);

}
