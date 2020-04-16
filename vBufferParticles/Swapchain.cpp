#include "Swapchain.h"

/// Queries the GPU implementation for specific support details.
void Swapchain::SupportDetails::operator()(const VkPhysicalDevice& physicalDevice, const VkSurfaceKHR& surface) {

	// get the capabilities of this GPU
	if (vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &capabilities) != VK_SUCCESS) {
		throw std::runtime_error("Swapchain support details: Could not get device surface capabilities");
	}

	// get the image formats supported by this GPU
	uint32_t formatCount;
	if (vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, NULL) != VK_SUCCESS) {
		throw std::runtime_error("Swapchain support details: Could not get surface formats");
	}
	if (formatCount != 0) {
		formats.resize(formatCount);
		if (vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, formats.data()) != VK_SUCCESS) {
			throw std::runtime_error("Swapchain support details: Could not get surface formats the second time");
		}
	}
	
	// get the present modes for the KHR surfaces supported by this GPU
	uint32_t presentModeCount;
	if (vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, NULL) != VK_SUCCESS) {
		throw std::runtime_error("Swapchain support details: Could not get surface present modes");
	}
	if (presentModeCount != 0) {
		presentModes.resize(presentModeCount);
		if (vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, presentModes.data()) != VK_SUCCESS) {
			throw std::runtime_error("Swapchain support details: Could not get surface present modes the second time");
		}
	}

}

/// Created the swapchain
Swapchain::Swapchain(DevicesPtr devices) : devices(devices) {

	// get support details on this GPU
	SupportDetails supportDetails(devices->getPhysicalDevice(), devices->getSurface());

	//Pick swapchain properties
	VkSurfaceFormatKHR surfaceFormat = pickSwapSurfaceFormat(supportDetails.formats);
	VkPresentModeKHR presentMode = pickSwapPresentMode(supportDetails.presentModes);
	VkExtent2D extent = pickSwapExtent(supportDetails.capabilities, devices->getWindowExtent());
	uint32_t imageCount = supportDetails.capabilities.minImageCount + 1;//request at least one more image than the minimum allowed
	if (supportDetails.capabilities.maxImageCount > 0 && imageCount > supportDetails.capabilities.maxImageCount) {
		imageCount = supportDetails.capabilities.maxImageCount;
	}

	//Create swapchain
	VkSwapchainCreateInfoKHR createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	createInfo.surface = devices->getSurface();
	createInfo.minImageCount = imageCount;
	createInfo.imageFormat = surfaceFormat.format;
	createInfo.imageColorSpace = surfaceFormat.colorSpace;
	createInfo.imageExtent = extent;
	createInfo.imageArrayLayers = 1;
	createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;//Image will be rendered directly to screen. Change to VK_IMAGE_USAGE_TRANSFER_DST_BIT for post-processing.
	uint32_t queueFamilyIndices[] = { devices->getGraphicsQueueFamily(), devices->getPresentQeueuFamily() };
	if (devices->getGraphicsQueueFamily() != devices->getPresentQeueuFamily()) {//different queues for graphics and presentation
		createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;//images can be shared across several queue families
		createInfo.queueFamilyIndexCount = 2;
		createInfo.pQueueFamilyIndices = queueFamilyIndices;
	} else {//same queue
		createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;//best performance
		createInfo.queueFamilyIndexCount = 0;//optional
		createInfo.pQueueFamilyIndices = NULL;//optional
	}
	createInfo.preTransform = supportDetails.capabilities.currentTransform;
	createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	createInfo.presentMode = presentMode;
	createInfo.clipped = VK_TRUE;//indicate pixels behind other windows don't matter
	createInfo.oldSwapchain = VK_NULL_HANDLE;//assume only one swapchain will be created

	if (vkCreateSwapchainKHR(*devices->getLogicalDevice(), &createInfo, NULL, &swapchain) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create swapchain");
	}

	// use the format and extent found (typically the current window size)
	format = surfaceFormat.format;
	this->extent = extent;

	//Retrieve swapchain images:
	if (vkGetSwapchainImagesKHR(*devices->getLogicalDevice(), swapchain, &imageCount, NULL) != VK_SUCCESS) {
		throw std::runtime_error("Could not get swapchain images");
	}
	images.resize(imageCount);
	if (vkGetSwapchainImagesKHR(*devices->getLogicalDevice(), swapchain, &imageCount, images.data()) != VK_SUCCESS) {
		throw std::runtime_error("Could not get swapchain images");
	}

	createImageViews();

}

// cleanup
Swapchain::~Swapchain() {

	// cleanup framebuffers
	for (auto framebuffer : framebuffers) {
		vkDestroyFramebuffer(*devices->getLogicalDevice(), framebuffer, NULL);
	}

	// cleanup image views
	for (int i = 0; i < imageViews.size(); ++i) {
		vkDestroyImageView(*devices->getLogicalDevice(), imageViews[i], NULL);
		imageViews[i] = NULL;
	}

	// cleanup swapchain KHR resource
	vkDestroySwapchainKHR(*devices->getLogicalDevice(), swapchain, NULL);
	swapchain = NULL;

}

/// Create framebuffer set for a renderpass, given the attachments that should be included. Attachments will be offset by one as the first attachment will always be the swapchain framebuffer.
void Swapchain::createFramebuffers(std::vector<VkImageView>& attachments, const VkRenderPass& renderPass) {

	framebuffers.resize(imageViews.size());
	for (int i = 0; i < imageViews.size(); ++i) {

		std::vector<VkImageView> fbAttachments = { imageViews[i] };//prepend current swapchain image view as attachment #0
		for (int i = 0; i < attachments.size(); ++i) {
			fbAttachments.push_back(attachments[i]);
		}

		VkFramebufferCreateInfo framebufferInfo = {};
		framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferInfo.renderPass = renderPass;
		framebufferInfo.attachmentCount = (uint32_t)fbAttachments.size();
		framebufferInfo.pAttachments = fbAttachments.data();
		framebufferInfo.width = extent.width;
		framebufferInfo.height = extent.height;
		framebufferInfo.layers = 1;

		if (vkCreateFramebuffer(*devices->getLogicalDevice(), &framebufferInfo, NULL, &framebuffers[i]) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create framebuffer");
		}
	}
}

/// Choose which surface image format to use from the available formats on the GPU
VkSurfaceFormatKHR Swapchain::pickSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) {

	for (const auto& format : availableFormats) {
		if (format.format == VK_FORMAT_B8G8R8A8_UNORM && format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
			return format;
		}
	}

	printf("[Warning] Device doesn't offer VK_FORMAT_B8G8R8A8_UNORM with VK_COLOR_SPACE_SRGB_NONLINEAR_KHR color space surface format; settling with first format.\n");
	return availableFormats[0];
}

/// Choose which present mode to use from the available ones on the GPU
VkPresentModeKHR Swapchain::pickSwapPresentMode(const std::vector<VkPresentModeKHR>& availableModes) {

	for (const auto& mode : availableModes) {
		if (mode == VK_PRESENT_MODE_MAILBOX_KHR) {
			return mode;
		}
	}

	printf("[Warning] Device doesn't offer VK_PRESENT_MODE_MAILBOX_KHR present mode; settling with VK_PRESENT_MODE_FIFO_KHR.\n");
	return VK_PRESENT_MODE_FIFO_KHR;
}

/// depending on GPU implementation, choose extents of swapchain (either UINT32_MAX pair, meaning the swapchain framebuffers will automatically be the size of the window; or manually pass the size of the window if it's allowed to differ)
VkExtent2D Swapchain::pickSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities, const VkExtent2D& windowExtent) {

	if (capabilities.currentExtent.width != UINT32_MAX) {//not allowed to differ from surface extent defined by window manager
		return capabilities.currentExtent;
	} else {//allowed to differ
		VkExtent2D actualExtent = windowExtent;
		actualExtent.width = std::max(capabilities.minImageExtent.width, std::min(capabilities.maxImageExtent.width, actualExtent.width));
		actualExtent.height = std::max(capabilities.minImageExtent.height, std::min(capabilities.maxImageExtent.height, actualExtent.height));
		return actualExtent;
	}
}

/// From the framebuffer images, create the image views available to shaders and GPU.
void Swapchain::createImageViews() {

	imageViews.resize(images.size());
	for (int i = 0; i < images.size(); ++i) {
		VkImageViewCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		createInfo.image = images[i];
		createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		createInfo.format = format;
		createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		createInfo.subresourceRange.baseMipLevel = 0;
		createInfo.subresourceRange.levelCount = 1;
		createInfo.subresourceRange.baseArrayLayer = 0;
		createInfo.subresourceRange.layerCount = 1;
		if (vkCreateImageView(*devices->getLogicalDevice(), &createInfo, NULL, &imageViews[i]) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create swapchain image view");
		}
	}
}
