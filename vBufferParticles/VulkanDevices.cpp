#include "VulkanDevices.h"

#include "Swapchain.h"
#include "StaticSettings.h"

VulkanDevices::VulkanDevices(const std::string& windowName, GLFWframebuffersizefun onFramebufferResized, void* glfwUserPointer, bool enableValidationLayers, const std::vector<const char*>& validationLayers, const std::vector<const char*>& deviceExtensions) {

	// Create glfw window, vk instance, surface, physical & logical device
	createWindow(windowName, onFramebufferResized, glfwUserPointer);
	createVulkanInstance(enableValidationLayers, validationLayers);
	if (glfwCreateWindowSurface(instance, window, NULL, &surface) != VK_SUCCESS) // create surface
		throw std::runtime_error("Failed to create window surface");
	pickPhysicalDevice(deviceExtensions);
	createLogicalDevice(enableValidationLayers, validationLayers, deviceExtensions);

}

VulkanDevices::~VulkanDevices() {
	//cleanup Vulkan
	vkDestroyDevice(logicalDevice, NULL);
	vkDestroySurfaceKHR(instance, surface, NULL);
	vkDestroyInstance(instance, NULL);

	//cleanup GLFW window
	glfwDestroyWindow(window);
	glfwTerminate();
}

void VulkanDevices::createWindow(const std::string& windowName, GLFWframebuffersizefun onFramebufferResized, void* glfwUserPointer) {

	// initialize GLFW
	glfwInit();

	// set flags for window creation
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);//Request that GLFW create no OGL context
	glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);//Allow resizing window

	// Select size and position
	int monitorCount;
	GLFWmonitor** monitors = glfwGetMonitors(&monitorCount);
	int monitorX, monitorY, monitorWidth, monitorHeight;
	if (monitorCount > 0) {
		glfwGetMonitorWorkarea(monitors[0], &monitorX, &monitorY, &monitorWidth, &monitorHeight);
	} else {
		monitorX = monitorY = monitorWidth = monitorHeight = 0; // get rid of uninitialized variable warnings :)
		throw std::runtime_error("Cannot get monitor work area.");
	}

	// Create window
	int windowWidth = WINDOW_WIDTH;
	int windowHeight = WINDOW_HEIGHT;
	if (RC_SETTINGS) {// take resolution from runtime constants if available
		windowWidth = RC_SETTINGS->windowWidth;
		windowHeight = RC_SETTINGS->windowHeight;
	}
	window = glfwCreateWindow(windowWidth, windowHeight, windowName.c_str(), NULL, NULL);
	glfwSetWindowPos(window, int((monitorWidth - windowWidth) * 0.5f), int((monitorHeight - windowHeight) * 0.5f));

	// Setup window resize callback
	glfwSetWindowUserPointer(window, glfwUserPointer);
	glfwSetFramebufferSizeCallback(window, onFramebufferResized);
}

void VulkanDevices::createVulkanInstance(bool enableValidationLayers, const std::vector<const char*>& validationLayers) {

	/// Check validation layer support
	if (enableValidationLayers && !checkValidationLayerSupport(validationLayers)) {
		throw std::runtime_error("Validation layers not available!");
	} else {
		printf("All requested validation layers available.\n");
	}

	//Required extensions for GLFW interfacing
	uint32_t glfwExtensionCount = 0;
	const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

	//Extension checking
	std::vector<bool> extensionOk;
	printf("Required extensions:\n");
	for (unsigned int i = 0; i < glfwExtensionCount; ++i) {
		extensionOk.push_back(false);
		printf("\t%s\n", glfwExtensions[i]);
	}

	//Check extension support
	printf("Extensions available:\n");
	uint32_t extensionCount = 0;
	vkEnumerateInstanceExtensionProperties(NULL, &extensionCount, NULL);
	std::vector<VkExtensionProperties> extensions(extensionCount);
	vkEnumerateInstanceExtensionProperties(NULL, &extensionCount, extensions.data());
	for (const auto& extension : extensions) {
		printf("\t%s ", extension.extensionName);
		for (unsigned int i = 0; i < glfwExtensionCount; ++i) {
			if (strcmp(glfwExtensions[i], extension.extensionName) == 0) {
				extensionOk[i] = true;
				printf("[OK]");
			}
		}
		printf("\n");
	}
	for (int i = 0; i < extensionOk.size(); ++i) {
		if (!extensionOk[i]) {
			printf("Error: Extension %s is not available!\n", glfwExtensions[i]);
		}
	}

	//Optional app info parameters
	VkApplicationInfo appInfo = {};
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pApplicationName = "vBuffer Particles";
	appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.pEngineName = "No Engine";
	appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.apiVersion = VK_API_VERSION_1_0;

	//Creation info parameters (layers, extensions)
	VkInstanceCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	createInfo.pApplicationInfo = &appInfo;
	createInfo.enabledExtensionCount = glfwExtensionCount;
	createInfo.ppEnabledExtensionNames = glfwExtensions;
	createInfo.enabledLayerCount = 0;
	if (enableValidationLayers) {
		createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
		createInfo.ppEnabledLayerNames = validationLayers.data();
	}

	//Create the instance
	if (vkCreateInstance(&createInfo, NULL, &instance) != VK_SUCCESS) {
		throw std::runtime_error("Could not create Vulkan instance!");
	}

}

bool VulkanDevices::checkValidationLayerSupport(const std::vector<const char*>& validationLayers) {

	uint32_t layerCount;
	vkEnumerateInstanceLayerProperties(&layerCount, NULL);

	std::vector<VkLayerProperties> availableLayers(layerCount);
	vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

	for (const char* layerName : validationLayers) {
		bool layerFound = false;

		for (const auto& layerProperties : availableLayers) {
			if (strcmp(layerName, layerProperties.layerName) == 0) {
				layerFound = true;
				printf("Found validation layer: %s\n", layerName);
				break;
			}
		}

		if (!layerFound) {
			printf("Layer %s not found!\n", layerName);
			return false;
		}
	}

	return true;
}

void VulkanDevices::pickPhysicalDevice(const std::vector<const char*>& deviceExtensions) {

	/// Enumerate GPUs
	uint32_t devicesCount = 0;
	if (vkEnumeratePhysicalDevices(instance, &devicesCount, NULL) != VK_SUCCESS) {
		throw std::runtime_error("Failed to enumerate physical devices");
	}
	if (devicesCount == 0) {
		throw std::runtime_error("Failed to find GPUs with Vulkan support");
	}
	std::vector<VkPhysicalDevice> devices(devicesCount);
	if (vkEnumeratePhysicalDevices(instance, &devicesCount, devices.data()) != VK_SUCCESS) {
		throw std::runtime_error("Failed to enumerate physical devices the second time");
	}

	printf("Found %d physical device(s)\n", devicesCount);

	/// Count suitable devices (only pick the first suitable one)
	int suitableDevices = 0;
	for (const auto& device : devices) {
		if (checkPhysicalDevice(device, deviceExtensions)) {
			if (physicalDevice == VK_NULL_HANDLE) physicalDevice = device;
			++suitableDevices;
		}
	}

	/// Check that at least one device was found
	if (physicalDevice == VK_NULL_HANDLE) {
		throw std::runtime_error("Failed to find suitable GPU for application to run");
	}

	printf("Found %d suitable physical device(s)\n", suitableDevices);

	// find queue families on device we picked
	queueFamilies = QueueFamilyIndices(physicalDevice, surface, true);

}

bool VulkanDevices::checkPhysicalDevice(const VkPhysicalDevice& device, const std::vector<const char*>& deviceExtensions) {

	VkPhysicalDeviceProperties deviceProperties;
	VkPhysicalDeviceFeatures deviceFeatures;

	vkGetPhysicalDeviceProperties(device, &deviceProperties);
	vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

	printf("\tChecking physical device: %s\n", deviceProperties.deviceName);

	bool suitable = true;

	//Device is a physical GPU
#ifdef PICK_PHYSICAL_GPU
	if (deviceProperties.deviceType != VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) suitable = false;
#else
	if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
		suitable = false;
		printf("\n\n\t\tWARNING: %s rejected because it is a physical GPU; #define PICK_PHYSICAL_GPU in VulkanDevices.h to use physical GPU instead of integrated chips.\n\n\n", deviceProperties.deviceName);
	}
#endif

	//Device has a geometry shader stage
	if (suitable && !deviceFeatures.geometryShader) suitable = false;

	//Device has the required device extensions
	if (suitable && !checkDeviceExtensionSupport(device, deviceExtensions)) suitable = false;

	//Device has the required queues
	if (suitable) {
		if (!QueueFamilyIndices(device, surface).isComplete()) suitable = false;
	}

	//Device has suitable swapchain
	if (suitable) {
		Swapchain::SupportDetails swapChainSupport(device, surface);
		suitable &= !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
	}

	//Device supports anisotropy
	if (suitable && !deviceFeatures.samplerAnisotropy) {
		suitable = false;
	}

	//Display maximum workgroup size
	printf("\t\t\tMaxComputeWorkGroupSize: (%d, %d, %d)\n", deviceProperties.limits.maxComputeWorkGroupSize[0], deviceProperties.limits.maxComputeWorkGroupSize[1], deviceProperties.limits.maxComputeWorkGroupSize[2]);
	//Display Geometry shader output limits
	printf("\t\t\tMaxGeometryOutputComponents: %d\n", deviceProperties.limits.maxGeometryOutputComponents);
	printf("\t\t\tMaxGeometryOutputVertices: %d\n", deviceProperties.limits.maxGeometryOutputVertices);
	printf("\t\t\tMaxGeometryTotalOutputComponents: %d\n", deviceProperties.limits.maxGeometryTotalOutputComponents);

	printf("\t\t> %s: %s\n", deviceProperties.deviceName, (suitable ? "[OK]" : "(unsuitable)"));

	return suitable;
}

bool VulkanDevices::checkDeviceExtensionSupport(const VkPhysicalDevice& device, const std::vector<const char*>& deviceExtensions) {

	uint32_t extensionCount;
	if (vkEnumerateDeviceExtensionProperties(device, NULL, &extensionCount, NULL) != VK_SUCCESS) {
		throw std::runtime_error("Failed to enumerate device extension properties");
	}
	std::vector<VkExtensionProperties> availableExtensions(extensionCount);
	if (vkEnumerateDeviceExtensionProperties(device, NULL, &extensionCount, availableExtensions.data()) != VK_SUCCESS) {
		throw std::runtime_error("Failed to enumerate device extension properties the second time");
	}

	std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());

	for (const auto& extension : availableExtensions) {
		requiredExtensions.erase(extension.extensionName);
	}

	return requiredExtensions.empty();
}

void VulkanDevices::createLogicalDevice(bool enableValidationLayers, const std::vector<const char*>& validationLayers, const std::vector<const char*>& deviceExtensions) {

	//Information for creating the queues (will only create one if all queues are the same)
	std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
	std::set<uint32_t> uniqueQueueFamilies = { queueFamilies.graphicsFamily.value(), queueFamilies.presentFamily.value() };
	float queuePriority = 1.0f;
	for (uint32_t queueFamily : uniqueQueueFamilies) {
		VkDeviceQueueCreateInfo queueCreateInfo = {};
		queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueCreateInfo.queueFamilyIndex = queueFamily;
		queueCreateInfo.queueCount = 1;
		queueCreateInfo.pQueuePriorities = &queuePriority;
		queueCreateInfos.push_back(queueCreateInfo);
	}

	//Device features we require
	VkPhysicalDeviceFeatures deviceFeatures = {};
	deviceFeatures.samplerAnisotropy = VK_TRUE;
	deviceFeatures.geometryShader = VK_TRUE;

	//Device creation info
	VkDeviceCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
	createInfo.pQueueCreateInfos = queueCreateInfos.data();
	createInfo.pEnabledFeatures = &deviceFeatures;
	createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
	createInfo.ppEnabledExtensionNames = deviceExtensions.data();
	//setup validation layers, although modern Vulkan ignores the following fields
	if (enableValidationLayers) {
		createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
		createInfo.ppEnabledLayerNames = validationLayers.data();
	} else {
		createInfo.enabledLayerCount = 0;
	}

	if (vkCreateDevice(physicalDevice, &createInfo, NULL, &logicalDevice) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create logical device!");
	}

	// Get the queues we need
	vkGetDeviceQueue(logicalDevice, queueFamilies.graphicsFamily.value(), 0, &graphicsQueue);
	vkGetDeviceQueue(logicalDevice, queueFamilies.presentFamily.value(), 0, &presentQueue);
	vkGetDeviceQueue(logicalDevice, queueFamilies.computeFamily.value(), 0, &computeQueue);
}
