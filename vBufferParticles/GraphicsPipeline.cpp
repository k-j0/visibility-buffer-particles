#include "GraphicsPipeline.h"

#include <cstdio>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <array>

//Compile all glsl shaders in the Shaders/ directory to SPIR-V bytecode (Windows-only!)
void CompileAllShaders() {
	printf("Compiling shaders to SPIR-V\n");
	/// Find all shader files in Shaders/ directory, compiles them and stores the Spir-V shaders in the CompiledShaders/ directory. Ignores the .glsl files (GLSL includes).
	std::filesystem::path shadersDir("Shaders/");
	std::filesystem::directory_iterator end_it;
	for (std::filesystem::directory_iterator it(shadersDir); it != end_it; ++it) {
		if (it->is_regular_file()) {
			std::string filename = it->path().string();
			CompileShader(filename);
		}
	}
	printf("\n");
}

/// Compiles one shader to Spir-V.
void CompileShader(std::string filename) {
	// check extension (only take into account files with a shader stage extension)
	if (filename.length() > 5) {
		std::string last5chars = filename.substr(filename.length() - 5, filename.length());
		std::string shaderFilename = "Compiled" + filename.substr(0, filename.length() - 5);
		std::string command = "";
		//check shader type from extension
		if (last5chars == ".vert") {// Vertex Shader
			printf(("\tCompiling vertex shader: " + filename + "\n").c_str());
			command = "vk-1.1.121.2\\Bin\\glslc.exe " + filename + " -o " + shaderFilename + "_v.spv";
		} else if (last5chars == ".frag") {// Fragment Shader
			printf(("\tCompiling fragment shader: " + filename + "\n").c_str());
			command = "vk-1.1.121.2\\Bin\\glslc.exe " + filename + " -o " + shaderFilename + "_f.spv";
		} else if (last5chars == ".geom") {// Geometry Shader
			printf(("\tCompiling geometry shader: " + filename + "\n").c_str());
			command = "vk-1.1.121.2\\Bin\\glslc.exe " + filename + " -o " + shaderFilename + "_g.spv";
		} else if (last5chars == ".comp") {// Compute Shader
			printf(("\tCompiling compute shader: " + filename + "\n").c_str());
			command = "vk-1.1.121.2\\Bin\\glslc.exe " + filename + " -o " + shaderFilename + "_c.spv";
		}
		//execute compile command with GLSLC (Google executable)
		if (command != "") {
			printf(("\t" + command + "\n").c_str());
			system(command.c_str());
		}
	}
}


/// Creates a shader module for a single stage, taking in the raw Spir-V code.
template<typename VertexType, VkPrimitiveTopology topology>
VkShaderModule GraphicsPipeline_Template<VertexType, topology>::createShaderModule(const std::vector<char>& code) {
	return U::createShaderModule(code, logicalDevice);
}

/// Gets shader stage info for a shader module, given a pipeline stage.
template<typename VertexType, VkPrimitiveTopology topology>
VkPipelineShaderStageCreateInfo GraphicsPipeline_Template<VertexType, topology>::getShaderStageInfo(const VkShaderModule& module, VkShaderStageFlagBits stage) {

	VkPipelineShaderStageCreateInfo info = {};
	info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	info.stage = stage;
	info.module = module;
	info.pName = ENTRYPOINT_SHADER_FUNCTION;//assuming all shaders start with the same function call

	return info;

}

/// Creates the graphics pipeline, given the shader filenames for the different stages.
template<typename VertexType, VkPrimitiveTopology topology>
GraphicsPipeline_Template<VertexType, topology>::GraphicsPipeline_Template(const std::string& vertexShaderFile, const std::string& fragmentShaderFile, const std::string* geometryShaderFile, const VkExtent2D& viewportSize, const VkPipelineLayout& pipelineLayout, const RenderPass* renderPass, uint32_t subpassId, bool depthWrite, uint32_t outputAttachmentCount, VkDevice* logicalDevice) : logicalDevice(logicalDevice) {

	ASSERT_IS_VERTEX_TYPE(VertexType)//assert that the template argument is a type derived from Vertex_Template


	// Create shader stages
	std::vector<VkPipelineShaderStageCreateInfo> shaderStages = {};

	auto vertShaderCode = U::readFile("CompiledShaders/" + vertexShaderFile + "_v.spv");
	VkShaderModule vertShaderModule = createShaderModule(vertShaderCode);
	shaderStages.push_back(getVertexShaderStageInfo(vertShaderModule));

	VkShaderModule geomShaderModule = VK_NULL_HANDLE;
	if (geometryShaderFile != NULL) {
		auto geomShaderCode = U::readFile("CompiledShaders/" + *geometryShaderFile + "_g.spv");
		geomShaderModule = createShaderModule(geomShaderCode);
		shaderStages.push_back(getGeometryShaderStageInfo(geomShaderModule));
	}

	auto fragShaderCode = U::readFile("CompiledShaders/" + fragmentShaderFile + "_f.spv");
	VkShaderModule fragShaderModule = createShaderModule(fragShaderCode);
	shaderStages.push_back(getFragmentShaderStageInfo(fragShaderModule));

	
	// Create input assembly stage

	auto vertBindingDesc = VertexType::getBindingDescription();
	auto vertAttrDesc = VertexType::getAttributeDescriptions();

	VkPipelineVertexInputStateCreateInfo vertInputInfo = {};
	vertInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertInputInfo.vertexBindingDescriptionCount = 1;
	vertInputInfo.pVertexBindingDescriptions = &vertBindingDesc;
	vertInputInfo.vertexAttributeDescriptionCount = (uint32_t)vertAttrDesc.size();
	vertInputInfo.pVertexAttributeDescriptions = vertAttrDesc.data();

	VkPipelineInputAssemblyStateCreateInfo iaInfo = {};
	iaInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	iaInfo.topology = topology;
	iaInfo.primitiveRestartEnable = VK_FALSE;

	
	// Create rasterizer stage

	VkViewport viewport = {};
	viewport.x = 0;
	viewport.y = 0;
	viewport.width = (float)viewportSize.width;
	viewport.height = (float)viewportSize.height;
	viewport.minDepth = 0;
	viewport.maxDepth = 1;

	VkRect2D scissor = {};
	scissor.offset = { 0, 0 };
	scissor.extent = viewportSize;

	VkPipelineViewportStateCreateInfo viewportInfo = {};
	viewportInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportInfo.viewportCount = 1;
	viewportInfo.pViewports = &viewport;
	viewportInfo.scissorCount = 1;
	viewportInfo.pScissors = &scissor;

	VkPipelineRasterizationStateCreateInfo rastInfo = {};
	rastInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rastInfo.depthClampEnable = VK_FALSE;
	rastInfo.rasterizerDiscardEnable = VK_FALSE;
	rastInfo.polygonMode = VK_POLYGON_MODE_FILL;
	rastInfo.lineWidth = 1.0f;
	rastInfo.cullMode = VK_CULL_MODE_BACK_BIT;
	rastInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	rastInfo.depthBiasEnable = VK_FALSE;


	// Create output merger stage
	
	VkPipelineMultisampleStateCreateInfo multiSampleInfo = {};
	multiSampleInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multiSampleInfo.sampleShadingEnable = VK_FALSE;
	multiSampleInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	multiSampleInfo.minSampleShading = 1.0f;
	multiSampleInfo.pSampleMask = NULL;
	multiSampleInfo.alphaToCoverageEnable = VK_FALSE;
	multiSampleInfo.alphaToOneEnable = VK_FALSE;

	std::vector<VkPipelineColorBlendAttachmentState> blendAttachments;
	for (unsigned int i = 0; i < outputAttachmentCount; ++i) {

		VkPipelineColorBlendAttachmentState blendAttachment = {};
		blendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
		blendAttachment.blendEnable = VK_FALSE;
		blendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
		blendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
		blendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
		blendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
		blendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
		blendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

		blendAttachments.push_back(blendAttachment);
	}

	VkPipelineColorBlendStateCreateInfo blendInfo = {};
	blendInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	blendInfo.logicOpEnable = VK_FALSE;
	blendInfo.logicOp = VK_LOGIC_OP_COPY;
	blendInfo.attachmentCount = (uint32_t)blendAttachments.size();
	blendInfo.pAttachments = blendAttachments.data();
	blendInfo.blendConstants[0] = blendInfo.blendConstants[1] = blendInfo.blendConstants[2] = blendInfo.blendConstants[3] = 0.0f;

	VkPipelineDepthStencilStateCreateInfo depthInfo = {};
	depthInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	depthInfo.depthTestEnable = depthWrite ? VK_TRUE : VK_FALSE;
	depthInfo.depthWriteEnable = depthWrite? VK_TRUE : VK_FALSE;
	depthInfo.depthCompareOp = VK_COMPARE_OP_LESS;
	depthInfo.depthBoundsTestEnable = VK_FALSE;
	depthInfo.stencilTestEnable = VK_FALSE;

	
	// Create pipeline

	VkGraphicsPipelineCreateInfo info = {};
	info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	info.stageCount = (uint32_t)shaderStages.size();
	info.pStages = shaderStages.data();
	info.pVertexInputState = &vertInputInfo;
	info.pInputAssemblyState = &iaInfo;
	info.pViewportState = &viewportInfo;
	info.pRasterizationState = &rastInfo;
	info.pMultisampleState = &multiSampleInfo;
	info.pDepthStencilState = &depthInfo;
	info.pColorBlendState = &blendInfo;
	info.pDynamicState = NULL;
	info.layout = pipelineLayout;
	info.renderPass = renderPass->getRenderPass();
	info.subpass = subpassId;
	info.basePipelineHandle = VK_NULL_HANDLE;
	info.basePipelineIndex = -1;

	if (vkCreateGraphicsPipelines(*logicalDevice, VK_NULL_HANDLE, 1, &info, NULL, &pipeline) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create graphics pipeline");
	}


	// Cleanup shader modules

	vkDestroyShaderModule(*logicalDevice, vertShaderModule, NULL);
	if (geometryShaderFile != NULL) vkDestroyShaderModule(*logicalDevice, geomShaderModule, NULL);
	vkDestroyShaderModule(*logicalDevice, fragShaderModule, NULL);
}



//Template pre-definitions
template GraphicsPipeline_Template<Vertex, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST>::GraphicsPipeline_Template(const std::string&, const std::string&, const std::string*, const VkExtent2D&, const VkPipelineLayout&, const RenderPass*, uint32_t, bool, uint32_t, VkDevice*);
template GraphicsPipeline_Template<VisibilityVertex, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST>::GraphicsPipeline_Template(const std::string&, const std::string&, const std::string*, const VkExtent2D&, const VkPipelineLayout&, const RenderPass*, uint32_t, bool, uint32_t, VkDevice*);
template GraphicsPipeline_Template<PointVertex, VK_PRIMITIVE_TOPOLOGY_POINT_LIST>::GraphicsPipeline_Template(const std::string&, const std::string&, const std::string*, const VkExtent2D&, const VkPipelineLayout&, const RenderPass*, uint32_t, bool, uint32_t, VkDevice*);
template GraphicsPipeline_Template<NulVertex, VK_PRIMITIVE_TOPOLOGY_POINT_LIST>::GraphicsPipeline_Template(const std::string&, const std::string&, const std::string*, const VkExtent2D&, const VkPipelineLayout&, const RenderPass*, uint32_t, bool, uint32_t, VkDevice*);
template GraphicsPipeline_Template<NulVertex, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST>::GraphicsPipeline_Template(const std::string&, const std::string&, const std::string*, const VkExtent2D&, const VkPipelineLayout&, const RenderPass*, uint32_t, bool, uint32_t, VkDevice*);
