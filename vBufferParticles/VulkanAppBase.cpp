#include "VulkanAppBase.h"

#include "StaticSettings.h"

/// Runs the application; only returns once the GLFW window is closed.
void VulkanAppBase::run() {

	/// Initialize Vulkan & GLFW resources
	initialize();

	/// Main application loop
	while (!glfwWindowShouldClose(devices->getWindow())) {
		glfwPollEvents();
		render();
	}

	/// Wait until last frame is rendered before cleaning up
	vkDeviceWaitIdle(*devices());

	/// Cleanup resources used by swapchain
	cleanupSwapchain();
}

/// Cleans up vk resources
VulkanAppBase::~VulkanAppBase() {

	// cleanup input singleton
	delete Input::getInstance(devices->getWindow());

	// cleanup default texture sampler
	vkDestroySampler(*devices(), sampler, NULL);

	// cleanup vulkan & glfw objects
	vkDestroyCommandPool(*devices(), commandPool, NULL);
	vkDestroyCommandPool(*devices(), computeCommandPool, NULL);
	DELETE(devices.devices);

}

/// Initialize resources. Called once only.
void VulkanAppBase::initialize() {

	// Re-compile shaders if requested
	if ((RECOMPILE_SHADERS && (!RC_SETTINGS || (RC_SETTINGS && RC_SETTINGS->recompileShaders))) || (RC_SETTINGS && RC_SETTINGS->recompileShaders)) {// weird logic combination here: if RECOMPILE_SHADERS is true, will only recompile if there's no settings or if the constant settings agree; if it's false, will only recompile if the constant settings say otherwise.
		CompileAllShaders();
	}

	// Initialize GLFW & Vulkan instance + devices
	devices.devices = new VulkanDevices("vBuffer Particles", framebufferResizeCallback, this, ENABLE_VALIDATION_LAYERS, validationLayers, deviceExtensions);

	swapchain = new Swapchain(devices);

	createCommandPools();
	createDescriptorPool();

	//Create depth buffer
	depthBuffer = new Texture(VK_FORMAT_D32_SFLOAT, swapchain->getExtent(), VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_IMAGE_ASPECT_DEPTH_BIT, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, devices(), devices->getPhysicalDevice(), *getCommandPool(), devices->getGraphicsQueue());

	// Create texture sampler
	sampler = Texture::createSampler(*devices());

	/// Initialize application-specific resources
	init();

	createCommandBuffers();
	createSyncObjects();
}

/// Update function, called each frame
void VulkanAppBase::update(uint32_t imageIndex) {

	/// Variables used to compute frame time and application time.
	static auto startTime = std::chrono::high_resolution_clock::now();
	static float previousFrame = 0;
	auto currentTime = std::chrono::high_resolution_clock::now();

	/// time since start of application
	float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();
	/// delta time for this frame
	float dt = time - previousFrame;

	{// scope in which timing information (dt & time) is correct

		// upon pressing ESC, quit application
		if (Input::getInstance(devices->getWindow())->isKeyDown(GLFW_KEY_ESCAPE)) {
			exit(0);
		}

		// update camera
		camera.Update(Input::getInstance(devices->getWindow()), dt);

		// application-specific update
		frame(imageIndex, dt, time);

	}

	previousFrame = time;

}

/// Called each frame to render image.
void VulkanAppBase::render() {

	VkResult result;

	vkWaitForFences(*devices(), 1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);

#ifdef SUBMIT_COMPUTE
	/// Submit compute queue
	VkSubmitInfo computeSubmitInfo = {};
	computeSubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	computeSubmitInfo.pNext = NULL;
	computeSubmitInfo.commandBufferCount = 1;
	computeSubmitInfo.pCommandBuffers = &computeCommandBuffer;
	if (vkQueueSubmit(devices->getComputeQueue(), 1, &computeSubmitInfo, computeFence) != VK_SUCCESS) {
		throw std::runtime_error("Failed to submit compute command buffer!");
	}
#endif

	/// Figure out which image we need to render to on this frame (resize swapchain if necessary)
	uint32_t imageIndex;
	result = vkAcquireNextImageKHR(*devices(), swapchain->getSwapchain(), UINT64_MAX, imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex);
	if (result == VK_ERROR_OUT_OF_DATE_KHR) {
		recreateSwapchain();//window has been resized
		return;
	} else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
		throw std::runtime_error("Failed to acquire next image");
	}

	/// Frame updates based on current image index
	update(imageIndex);

	/// Submit graphics queue
	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	VkSemaphore waitSemaphores[] = { imageAvailableSemaphores[currentFrame] };
	VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = waitSemaphores;
	submitInfo.pWaitDstStageMask = waitStages;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffers[imageIndex];
	VkSemaphore signalSemaphores[] = { renderFinishedSemaphores[currentFrame] };
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = signalSemaphores;
	vkResetFences(*devices(), 1, &inFlightFences[currentFrame]);
	if (vkQueueSubmit(devices->getGraphicsQueue(), 1, &submitInfo, inFlightFences[currentFrame]) != VK_SUCCESS) {
		throw std::runtime_error("Failed to submit draw command buffer");
	}

	/// Present swapchain to window
	VkPresentInfoKHR presentInfo = {};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = signalSemaphores;
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = &swapchain->getSwapchain();
	presentInfo.pImageIndices = &imageIndex;
	presentInfo.pResults = NULL;
	result = vkQueuePresentKHR(devices->getPresentQueue(), &presentInfo);
	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || framebufferResized) {
		framebufferResized = false;
		recreateSwapchain();//window has been resized
	} else if (result != VK_SUCCESS) {
		throw std::runtime_error("Failed to present queue");
	} else {
		// don't wait for compute fences if the swapchain has been recreated!

#ifdef SUBMIT_COMPUTE
		/// Any compute operation should be finalized here!
		vkWaitForFences(*devices(), 1, &computeFence, VK_TRUE, UINT64_MAX);
		vkResetFences(*devices(), 1, &computeFence);
#endif

	}

	++currentFrame;
	currentFrame %= MAX_FRAMES_IN_FLIGHT;

}

/// Callback upon GLFW window being resized.
void VulkanAppBase::framebufferResizeCallback(GLFWwindow* window, int width, int height) {
	// Grab handle to the VulkanAppBase object stored as part of the GLFW window
	auto vulkanApp = reinterpret_cast<VulkanAppBase*>(glfwGetWindowUserPointer(window));
	vulkanApp->framebufferResized = true;
}

/// Cleans up resources tied to swapchain.
void VulkanAppBase::cleanupSwapchain() {

	DELETE(depthBuffer);

	vkFreeCommandBuffers(*devices(), commandPool, static_cast<uint32_t>(commandBuffers.size()), commandBuffers.data());
	commandBuffers.clear();
	vkFreeCommandBuffers(*devices(), computeCommandPool, 1, &computeCommandBuffer);
	computeCommandBuffer = VK_NULL_HANDLE;

	DELETE(swapchain);

	vkDestroyDescriptorPool(*devices(), descriptorPool, NULL);
	descriptorPool = NULL;

	cleanupSyncObjects();
}

/// Recreates swapchain (upon window being resized for instance)
void VulkanAppBase::recreateSwapchain() {

	int width, height;
	do {//if the window has been minimized, wait until it's brought back on screen
		glfwGetFramebufferSize(devices->getWindow(), &width, &height);
		glfwWaitEvents();
	} while (width == 0 || height == 0);

	vkDeviceWaitIdle(*devices());

	// Cleanup resources
	cleanupSwapchain();

	swapchain = new Swapchain(devices);

	createDescriptorPool();

	//Create depth buffer
	depthBuffer = new Texture(VK_FORMAT_D32_SFLOAT, swapchain->getExtent(), VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_IMAGE_ASPECT_DEPTH_BIT, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, devices(), devices->getPhysicalDevice(), *getCommandPool(), devices->getGraphicsQueue());

	// Re-create application-specific resources once swapchain has been re-initialized
	onSwapchainResize();

	createCommandBuffers();

	createSyncObjects();
}

/// Creates command pool vk res.
void VulkanAppBase::createCommandPools() {

	/// Graphics command pool
	VkCommandPoolCreateInfo poolInfo = {};
	poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	poolInfo.queueFamilyIndex = devices->getGraphicsQueueFamily();
	poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	if (vkCreateCommandPool(*devices(), &poolInfo, NULL, &commandPool) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create command pool");
	}

	/// Compute command pool
	poolInfo.queueFamilyIndex = devices->getComputeQueueFamily();
	if (vkCreateCommandPool(*devices(), &poolInfo, NULL, &computeCommandPool) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create compute command pool");
	}

}

/// Creates descriptor pool vk res.
void VulkanAppBase::createDescriptorPool() {

	/// allow up 1000 of each descriptors used in application.
	std::vector<VkDescriptorPoolSize> poolSizes = {
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 100 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 25 },
		{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 20 },
		{ VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 20 }
	};

	VkDescriptorPoolCreateInfo poolInfo = {};
	poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolInfo.poolSizeCount = (uint32_t)poolSizes.size();
	poolInfo.pPoolSizes = poolSizes.data();
	poolInfo.maxSets = (uint32_t)poolSizes.size() * 1000;
	poolInfo.flags = 0;

	if (vkCreateDescriptorPool(*devices(), &poolInfo, NULL, &descriptorPool) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create descriptor pool.");
	}
}

/// Creates command buffers vk resources for swapchain images.
void VulkanAppBase::createCommandBuffers() {

	assert(commandBuffers.size() == 0);// check that command buffers didn't exist prior to this function
	assert(computeCommandBuffer == VK_NULL_HANDLE);

	commandBuffers.resize(swapchain->getSize());

	/// Allocate command buffers.
	VkCommandBufferAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool = commandPool;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandBufferCount = (uint32_t)commandBuffers.size();
	if (vkAllocateCommandBuffers(*devices(), &allocInfo, commandBuffers.data()) != VK_SUCCESS) {
		throw std::runtime_error("Failed to allocate command buffers");
	}

	/// Allocate compute command buffer
	allocInfo.commandPool = computeCommandPool;
	allocInfo.commandBufferCount = 1;
	if (vkAllocateCommandBuffers(*devices(), &allocInfo, &computeCommandBuffer) != VK_SUCCESS) {
		throw std::runtime_error("Failed to allocate compute command buffer!");
	}

	/// Record command buffers
	recordCommandBuffers();

}

/// Creates semaphores and gates used for rendering frames.
void VulkanAppBase::createSyncObjects() {

	imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
	renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
	inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

	VkSemaphoreCreateInfo semaphoreInfo = {};
	semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	VkFenceCreateInfo fenceInfo = {};
	fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

	for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
		if (vkCreateSemaphore(*devices(), &semaphoreInfo, NULL, &imageAvailableSemaphores[i]) != VK_SUCCESS ||
			vkCreateSemaphore(*devices(), &semaphoreInfo, NULL, &renderFinishedSemaphores[i]) != VK_SUCCESS ||
			vkCreateFence(*devices(), &fenceInfo, NULL, &inFlightFences[i]) != VK_SUCCESS) {

			throw std::runtime_error("Failed to create sync objects");
		}
	}

	fenceInfo.flags = 0;
	if (vkCreateFence(*devices(), &fenceInfo, NULL, &computeFence) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create sync objects");
	}

}

/// Cleans up semaphores and fences
void VulkanAppBase::cleanupSyncObjects() {
	vkDestroyFence(*devices(), computeFence, NULL);
	for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
		vkDestroySemaphore(*devices(), imageAvailableSemaphores[i], NULL);
		vkDestroySemaphore(*devices(), renderFinishedSemaphores[i], NULL);
		vkDestroyFence(*devices(), inFlightFences[i], NULL);
	}
}

/// Records all command buffers (must be initialized prior to calling)
void VulkanAppBase::recordCommandBuffers(bool waitIdle) {

	/// Wait for command buffers to be available first.
	if(waitIdle) vkDeviceWaitIdle(*devices());



	/// Record each command buffer individually, each with the same commands.
	for (int i = 0; i < commandBuffers.size(); ++i) {
		VkCommandBufferBeginInfo beginInfo = {};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = 0;
		beginInfo.pInheritanceInfo = NULL;

		if (vkBeginCommandBuffer(commandBuffers[i], &beginInfo) != VK_SUCCESS) {
			throw std::runtime_error("Failed to begin recording command buffer");
		}

		/// All application-specific recording happens here.
		recordCommandBuffer(commandBuffers[i], i);

		if (vkEndCommandBuffer(commandBuffers[i]) != VK_SUCCESS) {
			throw std::runtime_error("Failed to record command buffer!");
		}
	}


#ifdef SUBMIT_COMPUTE

	/// Record compute command buffer
	VkCommandBufferBeginInfo computeBeginInfo = {};
	computeBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	computeBeginInfo.pNext = NULL;
	computeBeginInfo.flags = 0;
	computeBeginInfo.pInheritanceInfo = NULL;

	if (vkBeginCommandBuffer(computeCommandBuffer, &computeBeginInfo) != VK_SUCCESS) {
		throw std::runtime_error("Failed to begin recording compute command buffer");
	}

	/// All application-specific recording happens here
	recordComputeCommandBuffer(computeCommandBuffer);

	if (vkEndCommandBuffer(computeCommandBuffer) != VK_SUCCESS) {
		throw std::runtime_error("Failed to record compute command buffer!");
	}

#endif

}

/// Immediately re-records command buffers.
void VulkanAppBase::Repaint() {
#ifdef PRINT_UPON_REPAINT
	static int repaints = -1;
	printf("\033[31mRepainting (%d)\033[0m\n", ++repaints);
#endif
	recordCommandBuffers();
}
