#include "UIOverlay.h"

bool Clicked = false;// flag set when clicking

UIOverlay::UIOverlay(DevicesPtr devices, std::function<void()> uiCallback) : devices(devices), uiCallback(uiCallback) {
	glfwSetMouseButtonCallback(devices->getWindow(), [](GLFWwindow* window, int button, int action, int mods) {
		if (action == GLFW_PRESS)
			Clicked = true;
		else
			Clicked = false;
	});
}

void UIOverlay::setupGUI(const VkDescriptorPool& descriptorPool, const VkCommandPool& commandPool, uint16_t swapchainSize, const RenderPass* renderPass) {

	// Cleanup if ImGui was already setup
	if (initialized) {
		ImGui_ImplVulkan_Shutdown();
		ImGui_ImplGlfw_Shutdown();
		ImGui::DestroyContext();
	}
	initialized = true;

	// Init ImGui
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	ImGui::StyleColorsLight();

	// Init vulkan/glfw implementation of imgui
	ImGui_ImplGlfw_InitForVulkan(devices->getWindow(), true);
	ImGui_ImplVulkan_InitInfo initInfo = {};
	initInfo.Instance = devices->getInstance();
	initInfo.PhysicalDevice = devices->getPhysicalDevice();
	initInfo.Device = *devices();
	initInfo.QueueFamily = devices->getGraphicsQueueFamily();
	initInfo.Queue = devices->getGraphicsQueue();
	initInfo.PipelineCache = NULL;
	initInfo.DescriptorPool = descriptorPool;
	initInfo.Allocator = NULL;
	initInfo.MinImageCount = 2;
	initInfo.ImageCount = swapchainSize;
	initInfo.CheckVkResultFn = check_vk_result;
	ImGui_ImplVulkan_Init(&initInfo, renderPass->getRenderPass(), renderPass->getSubpassCount() - 1);

	// Upload imgui fonts
	VkCommandBuffer cmdBuffer = U::beginSingleTimeCommands(commandPool, *devices(), devices->getGraphicsQueue()); {
		ImGui_ImplVulkan_CreateFontsTexture(cmdBuffer);
	} U::endSingleTimeCommands(cmdBuffer, commandPool, *devices(), devices->getGraphicsQueue());
	check_vk_result(vkDeviceWaitIdle(*devices()));
	ImGui_ImplVulkan_DestroyFontUploadObjects();

	// Initial update
	updateGUI();

	frames = 0;

}

UIOverlay::~UIOverlay() {

	/// Cleanup ImGui resources
	ImGui_ImplVulkan_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();

}

bool UIOverlay::updateGUI() {

	/// Update frame
	ImGui_ImplVulkan_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame(); {

		if (ImGui::Begin("Settings")) {

			/// All user-defined UI code
			uiCallback();


		} ImGui::End();

	} ImGui::Render();

	/// Check if rendered contents should be updated
	static int8_t repaintingFrames = 0;// counter for how many frames should be redrawn
	if (repaintingFrames > 0) --repaintingFrames;// decrease counter at each frame
	if (Clicked) {
		repaintingFrames = 6;// repaint for the next few frames
	}
	if (frames < 1) {
		++frames;
		return true;
	}

	return repaintingFrames > 0;

}

/// Bind ui overlay to command buffer
void UIOverlay::cmdBind(const VkCommandBuffer& cmdBuffer, int index) const {
	ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmdBuffer);
}
