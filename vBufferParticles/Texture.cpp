#include "Texture.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

/// Reads an image file using STB and fills a texture with the data
Texture::Texture(std::string path, VkDevice* logicalDevice, const VkPhysicalDevice& physicalDevice, const VkCommandPool& commandPool, const VkQueue& graphicsQueue) : logicalDevice(logicalDevice) {

	format = VK_FORMAT_R8G8B8A8_UNORM;// assume 8 bit RGBA values.

	//Read pixels from file
	int tWidth, tHeight, tChannels;
	stbi_uc* pixels = stbi_load(path.c_str(), &tWidth, &tHeight, &tChannels, STBI_rgb_alpha);
	VkDeviceSize imageSize = tWidth * tHeight * 4;

	if (!pixels) throw std::runtime_error("Failed to load pixel data from texture image");

	//Create staging buffer
	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;
	U::createBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory, *logicalDevice, physicalDevice);

	//Copy pixel data to vk buffer
	void* data;
	vkMapMemory(*logicalDevice, stagingBufferMemory, 0, imageSize, 0, &data); {
		memcpy(data, pixels, static_cast<size_t>(imageSize));
	} vkUnmapMemory(*logicalDevice, stagingBufferMemory);

	//Free up pixel memory
	stbi_image_free(pixels);

	//Create image object & bind memory
	createImage(tWidth, tHeight, format, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, textureImage, textureImageMemory, *logicalDevice, physicalDevice);

	//Copy memory from staging buffer to image object
	U::transitionImageLayout(textureImage, format, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, commandPool, *logicalDevice, graphicsQueue);
	U::copyBufferToImage(stagingBuffer, textureImage, tWidth, tHeight, *logicalDevice, commandPool, graphicsQueue);

	//Prepare the image for sampling from shaders
	U::transitionImageLayout(textureImage, format, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, commandPool, *logicalDevice, graphicsQueue);

	//Free up staging memory
	vkDestroyBuffer(*logicalDevice, stagingBuffer, NULL);
	vkFreeMemory(*logicalDevice, stagingBufferMemory, NULL);

	//Create default image view
	imageView = createImageView(*logicalDevice, format, VK_IMAGE_ASPECT_COLOR_BIT);

}

// create texture used as framebuffer attachment
Texture::Texture(VkFormat format, VkExtent2D size, VkImageUsageFlags usage, VkImageAspectFlags aspectFlags, VkImageLayout layout, VkDevice* logicalDevice, const VkPhysicalDevice& physicalDevice, const VkCommandPool& commandPool, const VkQueue& graphicsQueue) : logicalDevice(logicalDevice), format(format){

	// create image and image view.
	createImage(size.width, size.height, format, VK_IMAGE_TILING_OPTIMAL, usage, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, textureImage, textureImageMemory, *logicalDevice, physicalDevice);
	imageView = createImageView(*logicalDevice, format, aspectFlags);

	// transition image to required layout
	U::transitionImageLayout(textureImage, format, VK_IMAGE_LAYOUT_UNDEFINED, layout, commandPool, *logicalDevice, graphicsQueue);

}

Texture::~Texture(){
	// Cleanup vulkan objects
	vkDestroyImageView(*logicalDevice, imageView, NULL);
	vkDestroyImage(*logicalDevice, textureImage, NULL);
	vkFreeMemory(*logicalDevice, textureImageMemory, NULL);
}

void Texture::createImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory, VkDevice& logicalDevice, const VkPhysicalDevice& physicalDevice){
	//Create vk image object
	VkImageCreateInfo imageInfo = {};
	imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageInfo.imageType = VK_IMAGE_TYPE_2D;
	imageInfo.extent.width = width;
	imageInfo.extent.height = height;
	imageInfo.extent.depth = 1;
	imageInfo.mipLevels = 1;
	imageInfo.arrayLayers = 1;
	imageInfo.format = format;
	imageInfo.tiling = tiling;
	imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	imageInfo.usage = usage;
	imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	imageInfo.flags = 0;
	if (vkCreateImage(logicalDevice, &imageInfo, NULL, &image) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create image");
	}

	//allocate & bind memory for image object
	VkMemoryRequirements memReq;
	vkGetImageMemoryRequirements(logicalDevice, image, &memReq);
	VkMemoryAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memReq.size;
	allocInfo.memoryTypeIndex = U::pickMemoryType(memReq.memoryTypeBits, properties, physicalDevice);
	if (vkAllocateMemory(logicalDevice, &allocInfo, NULL, &imageMemory) != VK_SUCCESS) {
		throw std::runtime_error("Failed to allocate image memory");
	}
	vkBindImageMemory(logicalDevice, image, imageMemory, 0);

}

VkImageView Texture::createImageView(VkDevice logicalDevice, VkFormat format, VkImageAspectFlags aspectFlags){
	
	/// Create image view from image vk resource.
	VkImageViewCreateInfo viewInfo = {};
	viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	viewInfo.image = textureImage;
	viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	viewInfo.format = format;
	viewInfo.subresourceRange.aspectMask = aspectFlags;
	viewInfo.subresourceRange.baseMipLevel = 0;
	viewInfo.subresourceRange.levelCount = 1;
	viewInfo.subresourceRange.baseArrayLayer = 0;
	viewInfo.subresourceRange.layerCount = 1;
	VkImageView imageView;
	if (vkCreateImageView(logicalDevice, &viewInfo, NULL, &imageView) != VK_SUCCESS)
		throw std::runtime_error("Could not create image view for texture");

	return imageView;

}

/// Static function for creation of a default sampler resource
VkSampler Texture::createSampler(VkDevice logicalDevice){

	// default sampler params.
	VkSamplerCreateInfo samplerInfo = {};
	samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	samplerInfo.magFilter = samplerInfo.minFilter = VK_FILTER_LINEAR;
	samplerInfo.addressModeU = samplerInfo.addressModeV = samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.anisotropyEnable = VK_TRUE;
	samplerInfo.maxAnisotropy = 16;
	samplerInfo.unnormalizedCoordinates = VK_FALSE;
	samplerInfo.compareEnable = VK_FALSE;
	samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	samplerInfo.mipLodBias = 0.f;
	samplerInfo.minLod = 0.f;
	samplerInfo.maxLod = 0.f;

	VkSampler sampler;
	if (vkCreateSampler(logicalDevice, &samplerInfo, NULL, &sampler) != VK_SUCCESS)
		throw std::runtime_error("Cannot create texture sampler!");

	return sampler;

}

/// Returns a descriptor for the current texture, assuming a layout of READ_ONLY
VkDescriptorImageInfo Texture::getDescriptor(VkSampler sampler){

	VkDescriptorImageInfo info = {};
	info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	info.imageView = imageView;
	info.sampler = sampler;

	return info;

}

/// Returns a descriptor write entry for the texture to bind to a descriptor.
VkWriteDescriptorSet Texture::getDescriptorEntry(VkDescriptorImageInfo* descriptor, uint32_t binding, const VkDescriptorSet& descriptorSet) {

	VkWriteDescriptorSet entry = {};
	entry.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	entry.dstSet = descriptorSet;
	entry.dstBinding = binding;
	entry.dstArrayElement = 0;
	entry.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	entry.descriptorCount = 1;
	entry.pImageInfo = descriptor;
	entry.pBufferInfo = NULL;
	entry.pTexelBufferView = NULL;
	return entry;

}
