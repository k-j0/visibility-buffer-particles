#include "Descriptor.h"

/// Creates the descriptor set layout vulkan object.
void Descriptor::createDescriptorSetLayout(std::vector<std::pair<VkDescriptorType, VkShaderStageFlags>>& bindings) {

	/// Get the descriptor types and bindings from the argument.
	std::vector<VkDescriptorSetLayoutBinding> layoutBindings = {};
	int bindingIndex = -1;
	for (const std::pair<VkDescriptorType, VkShaderStageFlags>& binding : bindings) {
		VkDescriptorSetLayoutBinding layoutBinding = {};
		layoutBinding.binding = ++bindingIndex;
		layoutBinding.descriptorType = binding.first;
		layoutBinding.descriptorCount = 1;
		layoutBinding.stageFlags = binding.second;
		layoutBinding.pImmutableSamplers = NULL;
		layoutBindings.push_back(layoutBinding);
		descriptorTypes.push_back(binding.first);
	}

	/// Create the vulkan resource from the bindings.
	VkDescriptorSetLayoutCreateInfo layoutInfo = {};
	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.bindingCount = (uint32_t)layoutBindings.size();
	layoutInfo.pBindings = layoutBindings.data();
	if (vkCreateDescriptorSetLayout(*logicalDevice, &layoutInfo, NULL, &descriptorSetLayout) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create descriptor set layout.");
	}
}

/// (Re-)creates the pipeline layout vulkan resource.
void Descriptor::createPipelineLayout() {

	/// In case this function was previously called, destroy the old resource.
	if(pipelineLayout != VK_NULL_HANDLE)
		vkDestroyPipelineLayout(*logicalDevice, pipelineLayout, NULL);

	/// Create the pipeline layout vulkan resource from the descriptor set layout.
	VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = 1;
	pipelineLayoutInfo.pSetLayouts = &descriptorSetLayout;
	pipelineLayoutInfo.pushConstantRangeCount = 0;
	pipelineLayoutInfo.pPushConstantRanges = NULL;
	if (vkCreatePipelineLayout(*logicalDevice, &pipelineLayoutInfo, NULL, &pipelineLayout) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create pipeline layout!");
	}
}

/// Default constructor - simply creates the descriptor set layout.
Descriptor::Descriptor(std::vector<std::pair<VkDescriptorType, VkShaderStageFlags>>& bindings, VkDevice* logicalDevice, VkPipelineBindPoint pipelineBindPoint) :
			logicalDevice(logicalDevice), pipelineBindPoint(pipelineBindPoint) {
	createDescriptorSetLayout(bindings);
}

/// Cleanup resources.
Descriptor::~Descriptor() {
	vkDestroyDescriptorSetLayout(*logicalDevice, descriptorSetLayout, NULL);
	vkDestroyPipelineLayout(*logicalDevice, pipelineLayout, NULL);
}

/// (Re-)creates the descriptor sets for this descriptor. This function expects all uniform buffers and textures to have been initialized before being called.
void Descriptor::createDescriptorSets(int swapchainSize, const VkDescriptorPool& descriptorPool, std::vector<UBODescriptor> uboDescriptors, std::vector<ImageInfoDescriptor> imageDescriptors) {

	std::vector<VkDescriptorSetLayout> layouts(swapchainSize, descriptorSetLayout);

	/// Allocate descriptor sets
	VkDescriptorSetAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = descriptorPool;
	allocInfo.descriptorSetCount = (uint32_t)layouts.size();
	allocInfo.pSetLayouts = layouts.data();
	descriptorSets.resize(swapchainSize);
	if (vkAllocateDescriptorSets(*logicalDevice, &allocInfo, descriptorSets.data()) != VK_SUCCESS) {
		throw std::runtime_error("Failed to allocate descriptor sets.");
	}

	/// Creates the descriptor sets for each swapchain image.
	for (int i = 0; i < swapchainSize; ++i) {

		std::vector<VkWriteDescriptorSet> descriptorWrites;
		int bindingId = -1;// the current binding. Will start from 0 and go up by one each time.

		int bufferInfoId = -1;// the current ubo index
		int imageInfoId = -1;// the current image index

		for (const VkDescriptorType& type : descriptorTypes) {

			VkWriteDescriptorSet descriptorWrite = {};
			descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptorWrite.dstSet = descriptorSets[i];
			descriptorWrite.dstBinding = ++bindingId;
			descriptorWrite.dstArrayElement = 0;
			descriptorWrite.descriptorType = type;
			descriptorWrite.descriptorCount = 1;
			switch (type) {// depending on the type of resource, add a different type of descriptor write.
			case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
			case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:
				descriptorWrite.pBufferInfo = uboDescriptors[++bufferInfoId].getDescriptorInfo(i);//this pointer is guaranteed to remain valid until call to vkUpdateDescriptorSets, as its lifetime is the same as the UBODescriptor
				break;
			case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
			case VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT:
				{
					Descriptor::ImageInfoDescriptor& imgInfoDescriptor = imageDescriptors[++imageInfoId];
					VkDescriptorImageInfo* imgInfo = imgInfoDescriptor.getDescriptorInfo();
					descriptorWrite.pImageInfo = imgInfo;
				} break;
			default:
				throw std::runtime_error("Unsupported descriptor type: " + type);
			}

			descriptorWrites.push_back(descriptorWrite);

		}

		/// Creates the descriptor sets specified.
		vkUpdateDescriptorSets(*logicalDevice, (uint32_t)descriptorWrites.size(), descriptorWrites.data(), 0, NULL);

	}

}
