#pragma once

#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <fstream>
#include <vector>
#include <streambuf>
#include <vulkan/vulkan.hpp>

#define U Utils // quick access to class through U::

#ifdef NDEBUG
#define printf(...) // In Release mode, no need for outputting to cout.
#endif

/// Static class with helper functions
class Utils {
public:

	// cosine and sine of an angle in degrees
	static inline float cos(float degrees) { return cosf(degrees * 3.1415f/180.f); }
	static inline float sin(float degrees) { return sinf(degrees * 3.1415f / 180.f); }


	// returns  contents of a file
	static inline std::vector<char> readFile(const std::string& filename) {

		// open file
		std::ifstream file(filename, std::ios::ate | std::ios::binary);

		// check that file was opened
		if (!file.is_open()) {
			printf("File %s cannot be opened.\n", filename.c_str());
			throw std::runtime_error("Failed to open file!");
		}

		// get filesize
		size_t filesize = (size_t)file.tellg();
		std::vector<char> buffer(filesize);

		// record file contents from the beginning of the stream
		file.seekg(0);
		file.read(buffer.data(), filesize);

		// close file
		file.close();

		// return contents read from file.
		return buffer;
	}

	// returns contents of a file as a string
	static inline std::string readFileStr(const std::string& filename) {
		std::ifstream file(filename);
		return std::string((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
	}

	// overwrites a file's contents
	static inline void writeFile(const std::string& filename, const std::string& contents) {
		std::ofstream file(filename);
		file << contents;
	}

	// splits a string using a string delimiter
	// adapted from: https://stackoverflow.com/a/14266139
	static inline std::vector<std::string> splitStr(const std::string& delimiter, std::string str) {
		std::vector<std::string> results = {};
		size_t pos = 0;
		while ((pos = str.find(delimiter)) != std::string::npos) {
			results.push_back(str.substr(0, pos));
			str.erase(0, pos + delimiter.length());
		}
		results.push_back(str);
		return results;
	}


	// get memory types available to a GPU, and finds memory type suitable depending on the required filters passed.
	static inline uint32_t pickMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties, const VkPhysicalDevice& physicalDevice) {

		VkPhysicalDeviceMemoryProperties memoryProperties;
		vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memoryProperties);

		for (uint32_t i = 0; i < memoryProperties.memoryTypeCount; ++i) {
			if (typeFilter & (1 << i)) {
				if (memoryProperties.memoryTypes[i].propertyFlags & properties) {
					return i;
				}
			}
		}

		throw std::runtime_error("Could not find suitable memory type.");

	}

	/// Create a VkBuffer and VkBufferMemory bound to each other
	static inline void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory, const VkDevice& logicalDevice, const VkPhysicalDevice& physicalDevice) {

		//Create buffer

		VkBufferCreateInfo bufferInfo = {};
		bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferInfo.size = size;
		bufferInfo.usage = usage;
		bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		bufferInfo.flags = 0;

		if (vkCreateBuffer(logicalDevice, &bufferInfo, NULL, &buffer) != VK_SUCCESS) {
			throw std::runtime_error("Faield to create buffer!");
		}

		//Allocate memory

		VkMemoryRequirements memoryReq;
		vkGetBufferMemoryRequirements(logicalDevice, buffer, &memoryReq);

		VkMemoryAllocateInfo allocInfo = {};
		allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocInfo.allocationSize = memoryReq.size;
		allocInfo.memoryTypeIndex = pickMemoryType(memoryReq.memoryTypeBits, properties, physicalDevice);

		if (vkAllocateMemory(logicalDevice, &allocInfo, NULL, &bufferMemory) != VK_SUCCESS) {
			throw std::runtime_error("Failed to allocate buffer memory!");
		}

		// Bind allocated memory to buffer

		vkBindBufferMemory(logicalDevice, buffer, bufferMemory, 0);

	}

	/// Allocates command buffer for single time operations
	static inline VkCommandBuffer beginSingleTimeCommands(const VkCommandPool& commandPool, const VkDevice& logicalDevice, const VkQueue& graphicsQueue) {
		// allocate command on command pool

		VkCommandBufferAllocateInfo allocInfo = {};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandPool = commandPool;
		allocInfo.commandBufferCount = 1;

		VkCommandBuffer commandBuffer;
		vkAllocateCommandBuffers(logicalDevice, &allocInfo, &commandBuffer);

		VkCommandBufferBeginInfo beginInfo = {};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

		// start record command

		vkBeginCommandBuffer(commandBuffer, &beginInfo);

		return commandBuffer;
	}

	/// End single time operations begun with Utils::beginSingleTimeCommands
	static inline void endSingleTimeCommands(VkCommandBuffer commandBuffer, const VkCommandPool& commandPool, const VkDevice& logicalDevice, const VkQueue& graphicsQueue) {

		vkEndCommandBuffer(commandBuffer);

		// execute command buffer

		VkSubmitInfo submitInfo = {};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &commandBuffer;

		vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
		vkQueueWaitIdle(graphicsQueue);

		// free command buffer

		vkFreeCommandBuffers(logicalDevice, commandPool, 1, &commandBuffer);
	}

	/// Copy contents of a VkBuffer to another.
	static inline void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size, const VkCommandPool& commandPool, const VkDevice& logicalDevice, const VkQueue& graphicsQueue) {

		VkCommandBuffer commandBuffer = beginSingleTimeCommands(commandPool, logicalDevice, graphicsQueue); {
			VkBufferCopy copyRegion = {};
			copyRegion.srcOffset = 0;
			copyRegion.dstOffset = 0;
			copyRegion.size = size;
			vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);
		} endSingleTimeCommands(commandBuffer, commandPool, logicalDevice, graphicsQueue);

	}

	/// Transition image layouts of an image as a command
	static inline void cmdTransitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, VkCommandBuffer& cmdBuffer, VkDependencyFlags dependencyFlags) {

		// setup memory barrier for transition command
		VkImageMemoryBarrier barrier = {};
		barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		barrier.oldLayout = oldLayout;
		barrier.newLayout = newLayout;
		barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.image = image;
		barrier.subresourceRange.baseMipLevel = 0;
		barrier.subresourceRange.levelCount = 1;
		barrier.subresourceRange.baseArrayLayer = 0;
		barrier.subresourceRange.layerCount = 1;

		VkPipelineStageFlags srcStage;
		VkPipelineStageFlags dstStage;

		if (newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
			barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
		} else {
			barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		}


		/// pick pipeline stage flags and barrier flags depending on required transition
		
		// from UNDEFINED to TRANSFER_DST (for receiving transfer ops)
		if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
			barrier.srcAccessMask = 0;
			barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
			dstStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		}
		// from TRANSFER_DST to READ_ONLY (to be available to shaders as sampler 2d or input attachment)
		else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
			srcStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
			dstStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		}
		// from UNDEFINED to COLOR_ATTACHMENT (to be available to shaders as output attachment)
		else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL) {
			barrier.srcAccessMask = 0;
			barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
			srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
			dstStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		}
		// from UNDEFINED to DEPTH_STENCIL
		else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
			barrier.srcAccessMask = 0;
			barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
			srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
			dstStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
		}
		// Unsupported for now, simply throw exception.
		else {
			throw std::invalid_argument("Unsupported layout transition.");
		}

		// execute command
		vkCmdPipelineBarrier(cmdBuffer, srcStage, dstStage, dependencyFlags, 0, NULL, 0, NULL, 1, &barrier);

	}

	/// Transition image layout in a single time command
	static inline void transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, const VkCommandPool& commandPool, const VkDevice& logicalDevice, const VkQueue& graphicsQueue) {
		VkCommandBuffer cmdBuffer = beginSingleTimeCommands(commandPool, logicalDevice, graphicsQueue); {

			cmdTransitionImageLayout(image, format, oldLayout, newLayout, cmdBuffer, 0);

		} endSingleTimeCommands(cmdBuffer, commandPool, logicalDevice, graphicsQueue);
	}

	/// Copy contents of a VkBuffer to an image.
	static inline void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height, VkDevice logicalDevice, VkCommandPool commandPool, VkQueue graphicsQueue) {
		VkCommandBuffer cmdBuffer = beginSingleTimeCommands(commandPool, logicalDevice, graphicsQueue); {

			VkBufferImageCopy region = {};
			region.bufferOffset = 0;
			region.bufferRowLength = 0;
			region.bufferImageHeight = 0;
			region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			region.imageSubresource.mipLevel = 0;
			region.imageSubresource.baseArrayLayer = 0;
			region.imageSubresource.layerCount = 1;
			region.imageOffset = { 0, 0, 0 };
			region.imageExtent = { width, height, 1 };

			vkCmdCopyBufferToImage(cmdBuffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

		} endSingleTimeCommands(cmdBuffer, commandPool, logicalDevice, graphicsQueue);
	}

	/// Creates a shader module from the desired shader code
	static inline VkShaderModule createShaderModule(const std::vector<char>& code, VkDevice* logicalDevice) {
		VkShaderModuleCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		createInfo.pNext = NULL;
		createInfo.flags = 0;
		createInfo.codeSize = code.size();
		createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

		VkShaderModule shaderModule;
		if (vkCreateShaderModule(*logicalDevice, &createInfo, NULL, &shaderModule) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create shader module");
		}

		return shaderModule;
	}

};// class Utils

/// Delete + nullify macro.
#define DELETE(p) delete p; p = NULL
