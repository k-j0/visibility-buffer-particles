#pragma once

#include "Utils.h"
#include <vulkan/vulkan.hpp>
#include <vector>
#include <string>
#include <unordered_map>
#include <queue>
#include "Texture.h"
#include "Bindable.h"

/// Represents a pipeline layout specifying the attachments and uniforms that can be read from each shader stage in a specific subpass / graphics pipeline
class Descriptor : public Bindable {

	VkDevice* logicalDevice;// pointer to the current (unique) logical device
	VkDescriptorSetLayout descriptorSetLayout;
	std::vector<VkDescriptorType> descriptorTypes;
	VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
	std::vector<VkDescriptorSet> descriptorSets;
	VkPipelineBindPoint pipelineBindPoint;

	/// Creates the descriptor set layout given a certain amount of descriptor type + shader stage couples
	void createDescriptorSetLayout(std::vector<std::pair<VkDescriptorType, VkShaderStageFlags>>& bindings);

public:

	/// Represents a single descriptor for a uniform buffer. Used to create the descriptor set layout.
	struct UBODescriptor {

		std::vector<VkBuffer>& buffers;
		int size;
		std::vector<VkDescriptorBufferInfo> descriptorInfos;// cached descriptor info objects until the UBODescriptor goes out of scope
		
		// Takes the uniform buffers (from a UniformBuffer object) and the swapchain size to create a single Uniform Buffer Object descriptor
		inline UBODescriptor(std::vector<VkBuffer>& buf, int s) : buffers(buf), size(s) {
			// create descriptor infos ahead of time
			for (int i = 0; i < buf.size(); ++i) {
				VkDescriptorBufferInfo info = {};
				info.buffer = buffers[i];
				info.offset = 0;
				info.range = size;
				descriptorInfos.push_back(info);
			}
		}

		// Returns the DescriptorBufferInfo for a specific index in the swapchain.
		inline VkDescriptorBufferInfo* getDescriptorInfo(int index) {
			assert(index >= 0 && index < descriptorInfos.size());// in debug builds, crash here if the index is invalid.
			return &descriptorInfos[index];
		}

	};

	/// Represents a single descriptor for a texture attachment / sampler2D. Used to create the descriptor set layout.
	struct ImageInfoDescriptor {
		Texture* texture;
		VkSampler sampler;

		// Creates the image info descriptor, taking a texture object and a sampler. If the image is used as an input attachment, sampler can be VK_NULL_HANDLE.
		inline ImageInfoDescriptor(Texture* t, VkSampler sampler) : texture(t), sampler(sampler) {
			info = texture->getDescriptor(sampler);// let the texture object create its descriptor info.
		}

		// returns the descriptor info for this image descriptor.
		inline VkDescriptorImageInfo* getDescriptorInfo() {
			return &info;
		}

	private:
		VkDescriptorImageInfo info = {};
	};


	/// Creates a descriptor set from a set of descriptor type + shader stage pairs.
	Descriptor(std::vector<std::pair<VkDescriptorType, VkShaderStageFlags>>& bindings, VkDevice* logicalDevice, VkPipelineBindPoint pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS);

	/// Performs cleanup on the descriptor.
	virtual ~Descriptor();
	
	/// Creates the pipeline layout. Must be called each time the swapchain is recreated / resized.
	void createPipelineLayout();//happens each time upon recreation of swapchain

	/// Creates the descriptor sets for this Descriptor.
	void createDescriptorSets(int swapchainSize, const VkDescriptorPool& descriptorPool, std::vector<UBODescriptor> uboDescriptors, std::vector<ImageInfoDescriptor> imageDescriptors);

	
	/// Binds the attached pipeline layout and descriptor sets to the current command buffer
	inline void cmdBind(const VkCommandBuffer& cmdBuffer, int index) const override {
		vkCmdBindDescriptorSets(cmdBuffer, pipelineBindPoint, pipelineLayout, 0, 1, &descriptorSets[index], 0, NULL);
	}


	/// Getters

	/// The types of descriptors used by this object
	inline std::vector<VkDescriptorType>& getTypes() { return descriptorTypes; }// one of VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT for use within pool sizes
	/// The Vulkan descriptor set layout handle
	inline const VkDescriptorSetLayout& getDescriptorLayout() { return descriptorSetLayout; }
	/// The Vulkan pipeline layout handle
	inline VkPipelineLayout getPipelineLayout() { return pipelineLayout; }

};// class Descriptor


/// Shorthand for the type expected for the descriptor bindings sent to functions in the above class
#define DESCRIPTOR_BINDING_ARRAY std::vector<std::pair<VkDescriptorType, VkShaderStageFlags>>

/// The different types of descriptors typically used by the application
#define DESCRIPTOR_BINDING_UBO_VERTEX std::pair<VkDescriptorType, VkShaderStageFlags>{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT } // Uniform buffer object accessed from Vertex Shader
#define DESCRIPTOR_BINDING_UBO_FRAGMENT std::pair<VkDescriptorType, VkShaderStageFlags>{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT } // Uniform buffer object accessed from Fragment Shader
#define DESCRIPTOR_BINDING_UBO_GEOMETRY std::pair<VkDescriptorType, VkShaderStageFlags>{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_GEOMETRY_BIT } // Uniform buffer object accessed from Geometry Shader
#define DESCRIPTOR_BINDING_UBO_COMPUTE std::pair<VkDescriptorType, VkShaderStageFlags>{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT } // Uniform buffer object accessed from Compute Shader
#define DESCRIPTOR_BINDING_INPUT_ATTACHMENT_FRAGMENT std::pair<VkDescriptorType, VkShaderStageFlags>{ VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, VK_SHADER_STAGE_FRAGMENT_BIT } //  Input attachment accessed from Fragment Shader
#define DESCRIPTOR_BINDING_SAMPLER_FRAGMENT std::pair<VkDescriptorType, VkShaderStageFlags>{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT } // Sampler2D accessed from Fragment Shader
#define DESCRIPTOR_BINDING_STORAGE_BUFFER_COMPUTE std::pair<VkDescriptorType, VkShaderStageFlags>{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT } // Storage buffer written to by a Compute pass

/// Shorthand for an input attachment image info descriptor (note that for input attachments, sampler can be NULL_HANDLE as the pixels written to by the previous subpass will be the only available)
#define DESCRIPTOR_IMG_ATTACHMENT_INFO(attachment) Descriptor::ImageInfoDescriptor(attachment, VK_NULL_HANDLE) // no need for a sampler for input attachments, as they are read using subpassLoad()
