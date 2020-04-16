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

#define ENTRYPOINT_SHADER_FUNCTION "main" //assuming all shaders will start from main() here.


///Finds all glsl files in Shaders/ and compiles them to spir-v
void CompileAllShaders();
void CompileShader(std::string filename);// compiles a single shader to spir-v



/// Abstract base class for graphics pipelines
class GraphicsPipeline_Base : public Bindable {
public:
	inline virtual ~GraphicsPipeline_Base() {}
};// class GraphicsPipeline_Base


/// Represents a GraphicsPipeline with a specific vertex layout and primitive topology.
template<typename VertexType, VkPrimitiveTopology topology>
class GraphicsPipeline_Template : public GraphicsPipeline_Base {

	VkPipeline pipeline;// the vk res handle
	VkDevice* logicalDevice;


	/// Creates a single shader module (one shader stage)
	VkShaderModule createShaderModule(const std::vector<char>& code);

	/// Get info for creation of a shader pipeline
	VkPipelineShaderStageCreateInfo getShaderStageInfo(const VkShaderModule& module, VkShaderStageFlagBits stage);

	/// Helper functions for obtaining a specific shader stage info.
	inline VkPipelineShaderStageCreateInfo getVertexShaderStageInfo(const VkShaderModule& module) { return getShaderStageInfo(module, VK_SHADER_STAGE_VERTEX_BIT); }
	inline VkPipelineShaderStageCreateInfo getFragmentShaderStageInfo(const VkShaderModule& module) { return getShaderStageInfo(module, VK_SHADER_STAGE_FRAGMENT_BIT); }
	inline VkPipelineShaderStageCreateInfo getGeometryShaderStageInfo(const VkShaderModule& module) { return getShaderStageInfo(module, VK_SHADER_STAGE_GEOMETRY_BIT); }

public:

	/// Default constructor, builds a graphics pipeline from the specified shaders
	/// vertexShaderFile: filename for the vertex shader source
	/// fragmentShaderFile: filename for the fragment shader source
	/// geometryShaderFile: optional filename for the geometry shader source, or NULL
	/// viewportSize: the current size of the viewport
	/// pipelineLayout: the pipeline layout to use for this graphics pipeline
	/// renderPass: the renderPass this pipeline will be used in
	/// subpassId: the subpass index this pipeline will be used in
	/// depthWrite: whether this pipeline should write to depth buffer
	/// outputAttachmentCount: how many output attachments the fragment shader will be writing to
	/// logicalDevice: the current VkDevice.
	GraphicsPipeline_Template(const std::string& vertexShaderFile, const std::string& fragmentShaderFile, const std::string* geometryShaderFile, const VkExtent2D& viewportSize, const VkPipelineLayout& pipelineLayout, const RenderPass* renderPass, uint32_t subpassId, bool depthWrite, uint32_t outputAttachmentCount, VkDevice* logicalDevice);

	/// Cleans up Vulkan pipeline resource
	inline virtual ~GraphicsPipeline_Template() { vkDestroyPipeline(*logicalDevice, pipeline, NULL); }

	/// Binds the graphics pipeline to the command buffer at recording time.
	inline void cmdBind(const VkCommandBuffer& cmdBuffer, int index) const override { vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline); }


};// class GraphicsPipeline_Base

/// The different types of Graphics Pipelines used in the application
typedef GraphicsPipeline_Template<Vertex, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST> GraphicsPipeline;//default Graphics Pipeline type, supporting vertices with float3 position & normal, and float2 uv; a vertex and fragment shader.
typedef GraphicsPipeline_Template<VisibilityVertex, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST> VisibilityGraphicsPipeline;//Graphics Pipeline type for the visibility subpass, supporting vertices with float3 position, and float2 uv & ids; a vertex and fragment shader.
typedef GraphicsPipeline_Template<PointVertex, VK_PRIMITIVE_TOPOLOGY_POINT_LIST> PointGraphicsPipeline;// Graphics Pipeline type for point vertices (for use with geometry shaders)
typedef GraphicsPipeline_Template<NulVertex, VK_PRIMITIVE_TOPOLOGY_POINT_LIST> NulPointGraphicsPipeline;// Graphics Pipeline type for null vertices sent as points
typedef GraphicsPipeline_Template<NulVertex, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST> NulTriangleGraphicsPipeline;// Graphics Pipeline type for null vertices sent as triangle lists
