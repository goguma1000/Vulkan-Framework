#include "Renderer.h"
#include "Tools/PipelineBuilder.hpp"
#include "Tools/SamplerBuilder.hpp"
#include <cstdint>
#include <set>
#include <limits>
#include<algorithm>
#include <array>

using namespace Utils;
Renderer* Renderer::rendererInstance = nullptr;
bool Renderer::isInitialized = false;

Renderer::Renderer(GLFWwindow* wd, RendererCustomFuncs* funcs) : window(wd) {
	glfwSetWindowUserPointer(window, this);
	glfwSetFramebufferSizeCallback(window, FramebufferResizeCallback);
	if (funcs->checkSuitableDeviceFunc != nullptr) checkSuitableDeviceFunc = funcs->checkSuitableDeviceFunc;
	if (funcs->setPhysicalDeviceFeaturesFunc != nullptr) setPhysicalDeviceFeaturesFunc = funcs->setPhysicalDeviceFeaturesFunc;
	checkSwapPresentModeFunc = funcs->checkSwapPresentModeFunc;
	checkSwapSurfaceFormatFunc = funcs->checkSwapSurfaceFormatFunc;
	renderFunc = funcs->renderFunc;
	Init();
	if (rendererInstance == nullptr) {
		rendererInstance = this;
	}
}
Renderer* Renderer::GetInstance() {
	if (rendererInstance == nullptr) {
		std::cout << "Please create Renderer instance!\n";
		return nullptr;
	}
	else if (!isInitialized) {
		std::cout << "Please call Init() func in main(), before doing your work!\n";
		return nullptr;
	}
	else return rendererInstance;
}

Renderer* Renderer::GetInstance(GLFWwindow* window, RendererCustomFuncs* funcs) {
	if (rendererInstance == nullptr) {
		rendererInstance = new Renderer(window, funcs);
	}
	else return rendererInstance;
}
void Renderer::Init() {
	CreateVKinstance();
	SetupDebugMessenger();
	CreateSurface();
	PickFirstPhysicalDevice();
	CreateLogicalDevice();
	CreateSwapChain();
	CreateImageViews();
	PipelineBuilder::CreateDefaultRenderPass(defaultRenderpass, device, physicalDevice, swapChainImageFormat);
	CreateDefaultDescriptorSetLayout();
	CreateUniforBuffers();
	CreateDescriptorPool();
	CreateDescriptorSets();
	CreateDefaultSampler();
	PipelineBuilder::CreateDefaultGraphicsPipeline(defaultPipeline, defaultPipelineLayout, device, "DefaultVertexShader.spv", "DefaultFragmentShader.spv", defaultRenderpass, defaultDescriptorSetLayout);
	CreateCommandPool();
	CreateDepthResources();
	CreateFramebuffers();
	CreateCommandBuffers();
	CreateSyncObject();
	isInitialized = true;
}
void Renderer::Clean() {
	DestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
	CleanUpSwapChain();
	vkDestroyInstance(instance, nullptr);
}

void Renderer::CreateVKinstance() {
	VkApplicationInfo appInfo{};
	appInfo.sType= VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pApplicationName = "DonghoRenderer";
	appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.pEngineName = "DonghoEngine";
	appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.apiVersion = VK_API_VERSION_1_0;
	
	VkInstanceCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	createInfo.pApplicationInfo = &appInfo;

	std::vector<const char*> extensions = GetRequiredExtension(enableValidationLayer);
	createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
	createInfo.ppEnabledExtensionNames = extensions.data();
	
	if (enableValidationLayer && !CheckValidationLayerSupport(validationLayers)) {
		throw std::runtime_error("validation layers requested, but not available.");
	}
	
	VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo;
	if (enableValidationLayer) {
		createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
		createInfo.ppEnabledLayerNames = validationLayers.data();
		PopulateDebugMessengerCreateInfo(debugCreateInfo);
		createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo;
	}
	else {
		createInfo.enabledLayerCount = 0;
		createInfo.pNext = nullptr;
	}

	if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS) {
		throw std::runtime_error("failed to create instance!");
	}
}

void Renderer::SetupDebugMessenger() {
	if (!enableValidationLayer) return;
	VkDebugUtilsMessengerCreateInfoEXT createInfo;
	PopulateDebugMessengerCreateInfo(createInfo);
	if (CreateDebugUtilsMessengerEXT(instance, &createInfo, nullptr, &debugMessenger) != VK_SUCCESS) {
		throw std::runtime_error("failed to set up debug messenger!");
	}
}

void Renderer::CreateSurface() {
	if (glfwCreateWindowSurface(instance, window, nullptr, &surface) != VK_SUCCESS) {
		throw std::runtime_error("failed to create window surface!");
	}
}

bool Renderer::checkDeviceExtensionSupport(VkPhysicalDevice device) {
	uint32_t extensionCount;
	vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);
	std::vector<VkExtensionProperties> availableExtensions(extensionCount);
	vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

	std::set<std::string> requiredExtensions(deviceExtension.begin(), deviceExtension.end());
	std::cout << "Device extensions: \n";
	for (const auto& extension : availableExtensions) {
		std::cout << extension.extensionName << std::endl;
		requiredExtensions.erase(extension.extensionName);
	}

	return requiredExtensions.empty();
}

bool Renderer::IsDeviceSuitable(VkPhysicalDevice device) {
	QueueFamilyIndices temp = FindQueueFamiles(device, surface);
	bool extensionSupported = checkDeviceExtensionSupport(device);
	//after surface crate
	bool swapChainAdequate = false;
	if (extensionSupported) {
		SwapChainSupportDetails swapChainSupport = QuerrySwapChainSupport(device,surface);
		swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
	}
	return temp.isComplete()&& extensionSupported && swapChainAdequate && checkSuitableDeviceFunc(device); 
}

void Renderer::PickFirstPhysicalDevice() {
	uint32_t deviceCount(0);
	vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
	std::vector<VkPhysicalDevice>devices(deviceCount);
	vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());
	for (const auto& device : devices) {
		if (IsDeviceSuitable(device)) {
			physicalDevice = device;
			//indices = FindQueueFamiles(physicalDevice, surface);
			break;
		}
	}
	if (physicalDevice == VK_NULL_HANDLE) {
		throw std::runtime_error("failed to find a suitable GPU!");
	}
}

void Renderer::CreateLogicalDevice() {
	QueueFamilyIndices indices = FindQueueFamiles(physicalDevice, surface);

	std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
	std::set<uint32_t> uniqueQueueFamiles = { indices.graphicsFamily.value(), indices.presentFamily.value() };
	float queuePriority(1.0f);
	for (uint32_t queueFamily : uniqueQueueFamiles) {
		VkDeviceQueueCreateInfo queueCreateInfo{};
		queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueCreateInfo.queueFamilyIndex = queueFamily;
		queueCreateInfo.queueCount = 1;
		queueCreateInfo.pQueuePriorities = &queuePriority; // influence the scheduling of command buffer execution
		queueCreateInfos.push_back(queueCreateInfo);
	}

	VkPhysicalDeviceFeatures deviceFeatures{  };
	deviceFeatures.samplerAnisotropy = VK_TRUE;
	setPhysicalDeviceFeaturesFunc(deviceFeatures);
	VkDeviceCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	createInfo.pQueueCreateInfos = queueCreateInfos.data();
	createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
	createInfo.pEnabledFeatures = &deviceFeatures;
	createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtension.size());
	createInfo.ppEnabledExtensionNames = deviceExtension.data();

	if (enableValidationLayer) {
		createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
		createInfo.ppEnabledLayerNames = validationLayers.data();
	}
	else createInfo.enabledLayerCount = 0;

	if (vkCreateDevice(physicalDevice, &createInfo, nullptr, &device) != VK_SUCCESS) {
		throw std::runtime_error("failed to create logical device!");
	}
	vkGetDeviceQueue(device, indices.graphicsFamily.value(), 0, &graphicsQueue); //write 2024-08-15__03:10
	vkGetDeviceQueue(device, indices.presentFamily.value(), 0, &presentQueue); //write 2024-08-15__03:56.
	//In case the queue family are the same, two handles will most likely have the same value now.
}

VkSurfaceFormatKHR Renderer::ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) {
	bool isAvailable(false);
	for (const auto& availableFormat : availableFormats) {
		if (checkSwapSurfaceFormatFunc != nullptr) {
			if (checkSwapSurfaceFormatFunc(availableFormat)) isAvailable = true;
		}
		else if (availableFormat.format == VK_FORMAT_R8G8B8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
			isAvailable = true;
		}
		if (isAvailable) return availableFormat;
	}
	return availableFormats[0];
}

VkPresentModeKHR Renderer::ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes) {
	bool isAvailable(false);
	for (const auto& availablePresentMode : availablePresentModes) {
		if (checkSwapPresentModeFunc != nullptr) {
			if (checkSwapPresentModeFunc(availablePresentMode)) isAvailable = true;
		}
		else if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
			isAvailable = true;
		}
		if (isAvailable) return availablePresentMode;
	}
	return VK_PRESENT_MODE_FIFO_KHR;
}
VkExtent2D Renderer::ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) {
	
	if (capabilities.currentExtent.width != (std::numeric_limits<uint32_t>::max())) {
		return capabilities.currentExtent;
	}
	else {
		int width, height;
		glfwGetFramebufferSize(window, &width, &height);

		VkExtent2D actualExtent = {
			static_cast<uint32_t>(width),
			static_cast<uint32_t>(height)
		};
		actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
		actualExtent.height = std::clamp(actualExtent.width, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

		return actualExtent;
	}
}
void Renderer::CreateSwapChain() {
	SwapChainSupportDetails swapChainSupport = QuerrySwapChainSupport(physicalDevice, surface);
	VkSurfaceFormatKHR surfaceFormat = ChooseSwapSurfaceFormat(swapChainSupport.formats);
	VkPresentModeKHR presentMode = ChooseSwapPresentMode(swapChainSupport.presentModes);
	VkExtent2D extent = ChooseSwapExtent(swapChainSupport.capabilities);

	uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
	if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount) {
		imageCount = swapChainSupport.capabilities.maxImageCount;
	}
	VkSwapchainCreateInfoKHR createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	createInfo.surface = surface;
	createInfo.minImageCount = imageCount;
	createInfo.imageFormat = surfaceFormat.format;
	createInfo.imageColorSpace = surfaceFormat.colorSpace;
	createInfo.imageExtent = extent;
	createInfo.imageArrayLayers = 1;
	createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

	QueueFamilyIndices indices = FindQueueFamiles(physicalDevice, surface);
	uint32_t queueFamilyIndices[] = { indices.graphicsFamily.value(), indices.presentFamily.value() };
	if (indices.graphicsFamily != indices.presentFamily) {
		createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		createInfo.queueFamilyIndexCount = 2;
		createInfo.pQueueFamilyIndices = queueFamilyIndices;
	}
	else {
		createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		createInfo.queueFamilyIndexCount = 0;
		createInfo.pQueueFamilyIndices = nullptr;
	}
	createInfo.presentMode = presentMode;
	createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
	createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	createInfo.clipped = VK_TRUE;
	createInfo.oldSwapchain = VK_NULL_HANDLE;

	if (vkCreateSwapchainKHR(device, &createInfo, nullptr, &swapChain) != VK_SUCCESS) {
		throw std::runtime_error("failed to create swap chain!");
	}

	vkGetSwapchainImagesKHR(device, swapChain, &imageCount, nullptr);
	swapChainImages.resize(imageCount);
	vkGetSwapchainImagesKHR(device, swapChain, &imageCount, swapChainImages.data());

	swapChainImageFormat = surfaceFormat.format;
	swapChainExtent = extent;
}

void Renderer::CreateImageViews() {
	swapChainImageViews.resize(swapChainImages.size());
	for (size_t i = 0; i < swapChainImageViews.size(); ++i) {
		swapChainImageViews[i] =
			CreateImageView(
				device,
				swapChainImages[i],
				swapChainImageFormat,
				VK_IMAGE_VIEW_TYPE_2D,
				VK_IMAGE_ASPECT_COLOR_BIT,
				1
			);
	}
}

//custom yourself. if you add Descriptorset
void Renderer::CreateDefaultDescriptorSetLayout() {
	VkDescriptorSetLayoutBinding uboLayoutBinding = Initializer::InitDescriptorSetLayoutBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT);
	VkDescriptorSetLayoutBinding samplerLayoutBinding;
	std::vector<VkDescriptorSetLayoutBinding> bindings = { uboLayoutBinding };
	for (int i = 0; i < MAX_NUM_TEXTURE_BINDING; i++) {
		samplerLayoutBinding = Initializer::InitDescriptorSetLayoutBinding(1 + i, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT);
		bindings.push_back(samplerLayoutBinding);
	}
	//re-write after create sampler
	VkDescriptorSetLayoutCreateInfo layoutInfo = Initializer::InitDescriptorSetLayoutCreateInfo(static_cast<uint32_t>(bindings.size()), bindings.data());
	if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &defaultDescriptorSetLayout) != VK_SUCCESS) {
		throw std::runtime_error("failed to create descriptor set layout");
	}
}

//custom yourself. if you add Descriptorset
void Renderer::CreateDescriptorPool() {
	std::vector<VkDescriptorPoolSize> poolSizes(1);
	poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	poolSizes[0].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
	for (int i = 0; i < MAX_NUM_TEXTURE_BINDING; i++) {
		poolSizes.emplace_back();
		int idx = i + 1;
		poolSizes[idx].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		poolSizes[idx].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
	}
	VkDescriptorPoolCreateInfo poolInfo = Initializer::InitDescriptorPoolCreateInfo(static_cast<uint32_t>(poolSizes.size()),poolSizes.data(), static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT));	
	if (vkCreateDescriptorPool(device, &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS) {
		throw std::runtime_error("failed to create descriptor pool!");
	}
}

//custom yourself. if you add Descriptorset
void Renderer::CreateDescriptorSets() {
	std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, defaultDescriptorSetLayout);
	VkDescriptorSetAllocateInfo allocInfo = Initializer::InitDescriptorSetAllocateInfo(descriptorPool,static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT),layouts.data());
	descriptorSets.resize(MAX_FRAMES_IN_FLIGHT);
	if (vkAllocateDescriptorSets(device, &allocInfo, descriptorSets.data()) != VK_SUCCESS) {
		throw std::runtime_error("failed to allocate descriptor sets!");
	}
	// update descriptorset code
	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
		VkDescriptorBufferInfo bufferInfo = Initializer::InitDescriptorBufferInfo(uniformBuffers[i], sizeof(UniformBufferObject), 0);
		std::array<VkWriteDescriptorSet, 1> descriptorWrites;
		descriptorWrites[0] = Initializer::InitWriteDescriptorSet(descriptorSets[i], 0, 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, &bufferInfo);
		
		vkUpdateDescriptorSets(device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
	}
}

void Renderer::CreateUniforBuffers() {
	VkDeviceSize buffersize = sizeof(UniformBufferObject);
	uniformBuffers.resize(MAX_FRAMES_IN_FLIGHT);
	uniformBuffersMemory.resize(MAX_FRAMES_IN_FLIGHT);
	uniformBuffersMapped.resize(MAX_FRAMES_IN_FLIGHT);

	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
		CreateBuffer(device, physicalDevice, buffersize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, uniformBuffers[i], uniformBuffersMemory[i]);
		vkMapMemory(device, uniformBuffersMemory[i], 0, buffersize, 0, &uniformBuffersMapped[i]); //persistent mapping
	}
}

void Renderer::CreateDefaultSampler() {
	VkPhysicalDeviceProperties properties{};
	vkGetPhysicalDeviceProperties(physicalDevice, &properties);
	//max mipmap 8k image
	VkSamplerCreateInfo samplerInfo = SamplerBuilder::InitSamplerCreateInfo(14, 0.0f, 0.0f, VK_SAMPLER_MIPMAP_MODE_LINEAR, VK_TRUE, properties.limits.maxSamplerAnisotropy);
	SamplerBuilder::CreateSampler(device, defaultSampler, samplerInfo);
}

void Renderer::CreateDepthResources() {
	VkFormat depthFormat = findDepthFormat(physicalDevice);
	VkImageCreateInfo imageInfo =  Initializer::InitImageCreateInfo(VK_IMAGE_TYPE_2D, swapChainExtent.width, swapChainExtent.height, 1, 1, depthFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT);
	CreateImage(device, physicalDevice, depthImage, depthImageMemory, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, imageInfo);
	depthImageview = CreateImageView(device, depthImage, depthFormat, VK_IMAGE_VIEW_TYPE_2D, VK_IMAGE_ASPECT_DEPTH_BIT,1);
	transitionImageLayout(device, commandPool, graphicsQueue, depthImage, depthFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, 1);
}

void Renderer::CreateFramebuffers() {
	swapChainFramebuffers.resize(swapChainImageViews.size());
	for (int i = 0; i < swapChainFramebuffers.size(); i++) {
		std::vector<VkImageView> attachments = { swapChainImageViews[i], depthImageview};
		CreateFrameBuffer(swapChainFramebuffers[i], device, attachments, defaultRenderpass, swapChainExtent);
	}
}

void Renderer::CreateCommandPool() {
	QueueFamilyIndices queueFamilyIndices = FindQueueFamiles(physicalDevice, surface);

	VkCommandPoolCreateInfo poolInfo{};
	poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();
	if (vkCreateCommandPool(device, &poolInfo, nullptr, &commandPool) != VK_SUCCESS) {
		throw std::runtime_error("failed to create command pool!");
	}
}
void Renderer::CreateCommandBuffers() {
	commandBuffers.resize(MAX_FRAMES_IN_FLIGHT);
	VkCommandBufferAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool = commandPool;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandBufferCount = (uint32_t)commandBuffers.size();

	if (vkAllocateCommandBuffers(device, &allocInfo, commandBuffers.data()) != VK_SUCCESS) {
		throw std::runtime_error("failed to create command buffers!");
	}
}

void Renderer::CreateSyncObject() {
	imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
	renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
	inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

	VkSemaphoreCreateInfo semaphoreInfo = Initializer::InitSemaphoreCreateInfo();
	// On first frame, which immediately waits on inFlightFence to be signaled.
	// inFlightFence is only signaled after a frame has finished rendering
	// ,yet since this is the first frame, so vkWaitForFence blocks indefinitly.
	// one ofe the many solutions is create fence in the signaled state.
	VkFenceCreateInfo fenceInfo = Initializer::InitFenceCreateInfo(VK_FENCE_CREATE_SIGNALED_BIT);
	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
		if (vkCreateSemaphore(device, &semaphoreInfo, nullptr, &imageAvailableSemaphores[i]) != VK_SUCCESS
			||
			vkCreateSemaphore(device, &semaphoreInfo, nullptr, &renderFinishedSemaphores[i]) != VK_SUCCESS
			||
			vkCreateFence(device, &fenceInfo, nullptr, &inFlightFences[i]) != VK_SUCCESS) {
			throw std::runtime_error("failed to create semaphores!");
		}
	}
}

void Renderer::Render() {
	vkWaitForFences(device, 1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);

	uint32_t imageIdx;
	VkResult result = vkAcquireNextImageKHR(device, swapChain, UINT64_MAX, imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, &imageIdx);
	if (result == VK_ERROR_OUT_OF_DATE_KHR) {
		RecreateSwapChain();
		std::cout << "Out of Time!\n";
		return;
	}
	else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
		throw std::runtime_error("failed to acquire swap chain image!");
	}
	vkResetFences(device, 1, &inFlightFences[currentFrame]); //Delay resetting the fence until after we know for sure we will be submitting work with it.

	vkResetCommandBuffer(commandBuffers[currentFrame], 0);
	renderFunc(commandBuffers[currentFrame],swapChainFramebuffers[imageIdx],currentFrame);
	//updateUniformBuiffer(currentframe);
	VkSemaphore waitSemaphores[] = { imageAvailableSemaphores[currentFrame] };
	VkPipelineStageFlags waitStage[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
	VkSemaphore signalSemaphores[] = { renderFinishedSemaphores[currentFrame] };
	VkSubmitInfo submitInfo = Initializer::InitSubmitInfo(1, waitSemaphores,waitStage,1, &commandBuffers[currentFrame], 1, signalSemaphores);
	if (vkQueueSubmit(graphicsQueue, 1, &submitInfo, inFlightFences[currentFrame]) != VK_SUCCESS) {
		throw std::runtime_error("failed to submit draw command buffer!");
	}
	VkPresentInfoKHR presentInfo = Initializer::InitPresentInfo(1, signalSemaphores, 1, &swapChain, &imageIdx);
	result = vkQueuePresentKHR(presentQueue, &presentInfo);
	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || framebufferResized) {
		framebufferResized = false;
		RecreateSwapChain();
	}
	else if (result != VK_SUCCESS) {
		throw std::runtime_error("failed to present swap chain image!");
	}
	currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

void Renderer::CleanUpSwapChain() {
	for (size_t i = 0; i < swapChainFramebuffers.size(); i++) {
		vkDestroyFramebuffer(device, swapChainFramebuffers[i], nullptr);
	}
	for (size_t i = 0; i < swapChainImageViews.size(); i++) {
		vkDestroyImageView(device, swapChainImageViews[i], nullptr);
	}
	vkDestroySwapchainKHR(device, swapChain, nullptr);
}
void Renderer::RecreateSwapChain() {
	vkDeviceWaitIdle(device);

	CleanUpSwapChain();

	CreateSwapChain();
	CreateImageViews();
	CreateDepthResources();
	CreateFramebuffers();
}

void Renderer::UpdateUniformBuffer(uint32_t currentFrame, Utils::UniformBufferObject& ubo) {
	memcpy(uniformBuffersMapped[currentFrame], &ubo, sizeof(ubo));
}

#pragma region callback Function
void Renderer::FramebufferResizeCallback(GLFWwindow* window, int width, int height) {
	auto app = reinterpret_cast<Renderer*>(glfwGetWindowUserPointer(window));
	app->framebufferResized = true;
}
#pragma endregion