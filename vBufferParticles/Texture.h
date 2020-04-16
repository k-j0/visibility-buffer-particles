#pragma once

#include <string>
#include <vulkan/vulkan.hpp>
#include "Utils.h"

/// Represents a texture with its set of image, image memory, and image view. A texture can be either read from an image file or created as an attachment to the swapchain.
class Texture {

public:

	// Create texture from image file
	Texture(std::string path, VkDevice* logicalDevice, const VkPhysicalDevice& physicalDevice, const VkCommandPool& commandPool, const VkQueue& graphicsQueue);

	// Create texture from swapchain size
	Texture(VkFormat format, VkExtent2D size, VkImageUsageFlags usage, VkImageAspectFlags aspectFlags, VkImageLayout layout, VkDevice* logicalDevice, const VkPhysicalDevice& physicalDevice, const VkCommandPool& commandPool, const VkQueue& graphicsQueue);

	// Resource cleanup
	~Texture();

	/// Static function for creation of a default sampler
	static VkSampler createSampler(VkDevice logicalDevice);

	/// Get descriptor info and descriptor write entry to bind to Descriptor sets.
	VkDescriptorImageInfo getDescriptor(VkSampler sampler);
	VkWriteDescriptorSet getDescriptorEntry(VkDescriptorImageInfo* descriptor, uint32_t binding, const VkDescriptorSet& descriptorSet);

	/// Getters of the Vk resource handles
	inline const VkImageView& getImageView() { return imageView; }
	inline const VkImage& getImage() { return textureImage; }
	inline const VkFormat& getFormat() { return format; }

	// a tag accessible in Debug builds to identify texture objects. Set using SET_DEBUG_TEXTURE_IDENTIFIER macro.
#ifndef NDEBUG
	std::string debug_identifier = "unnamed texture";
#endif

private:

	/// Helper function to create an image with specified properties
	static void createImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory, VkDevice& logicalDevice, const VkPhysicalDevice& physicalDevice);

	/// Takes care of image view creation.
	VkImageView createImageView(VkDevice logicalDevice, VkFormat format, VkImageAspectFlags aspectFlags);


	VkDevice* logicalDevice;

	/// Resources for this texture
	VkImage textureImage;
	VkDeviceMemory textureImageMemory;
	VkImageView imageView;
	VkFormat format;// the image format.

};// class Texture

/// Macro to allow setting a debug identifier to specific textures in Debug builds to make debugging specific textures easier.
#ifdef NDEBUG
#define SET_DEBUG_TEXTURE_IDENTIFIER(id, tex)
#else
#define SET_DEBUG_TEXTURE_IDENTIFIER(id, tex) tex->debug_identifier = id
#endif
