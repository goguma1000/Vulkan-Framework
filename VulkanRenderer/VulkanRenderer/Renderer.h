#pragma once

#define GLFW_INCLUDE_VULKAN
#include<GLFW/glfw3.h>
#include<vector>
#include <stdexcept>
#include <functional>
#include "Tools/Utils.hpp"
struct RendererCustomFuncs {
	std::function<bool(VkPhysicalDevice device)> checkSuitableDeviceFunc = nullptr;
	std::function<void(VkPhysicalDeviceFeatures& deviceFeatures)> setPhysicalDeviceFeaturesFunc = nullptr;
	std::function<bool(const VkSurfaceFormatKHR& availableFormat)>checkSwapSurfaceFormatFunc = nullptr;
	std::function<bool(const VkPresentModeKHR& availableFormat)>checkSwapPresentModeFunc = nullptr;
	std::function<void(VkCommandBuffer, VkFramebuffer, uint32_t)> renderFunc = nullptr;
};

class Renderer {

public:
	std::function<bool(VkPhysicalDevice device)> checkSuitableDeviceFunc = [](VkPhysicalDevice device) {return false;};
	std::function<void(VkPhysicalDeviceFeatures& deviceFeatures)> setPhysicalDeviceFeaturesFunc = [](VkPhysicalDeviceFeatures& deviceFeatures) {};
	std::function<bool(const VkSurfaceFormatKHR& availableFormat)>checkSwapSurfaceFormatFunc = nullptr;
	std::function<bool(const VkPresentModeKHR& availableFormat)>checkSwapPresentModeFunc = nullptr;
	const std::vector<const char*> deviceExtension = {
		VK_KHR_SWAPCHAIN_EXTENSION_NAME
	};
	std::function<void(VkCommandBuffer, VkFramebuffer, uint32_t)> renderFunc = nullptr;

public:
	VkPhysicalDevice physicalDevice = { VK_NULL_HANDLE };
	VkDevice device = { VK_NULL_HANDLE };
	VkQueue graphicsQueue = { VK_NULL_HANDLE };
	VkQueue presentQueue = { VK_NULL_HANDLE };
	VkCommandPool commandPool = { VK_NULL_HANDLE };
	
private:
#ifdef NDEBUG
	const bool enableValidationLayer = false;
#else
	const bool enableValidationLayer = true;
#endif
	static Renderer* rendererInstance;

	static bool isInitialized;
	const std::vector<const char*> validationLayers = {
		"VK_LAYER_KHRONOS_validation"
	};
	GLFWwindow* window = nullptr;
	VkInstance instance = {VK_NULL_HANDLE};
	VkDebugUtilsMessengerEXT debugMessenger = { VK_NULL_HANDLE };
	VkSurfaceKHR surface = { VK_NULL_HANDLE };
	VkSwapchainKHR swapChain = { VK_NULL_HANDLE };
	std::vector<VkImage> swapChainImages;
	VkFormat swapChainImageFormat = VK_FORMAT_UNDEFINED;
	VkExtent2D swapChainExtent = {0,0};
	std::vector<VkImageView> swapChainImageViews;
	VkRenderPass defaultRenderpass = { VK_NULL_HANDLE };
	VkDescriptorSetLayout defaultDescriptorSetLayout = VK_NULL_HANDLE;
	VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
	std::vector<VkDescriptorSet>descriptorSets;
	VkSampler defaultSampler = VK_NULL_HANDLE;
	VkPipeline defaultPipeline = { VK_NULL_HANDLE };
	VkPipelineLayout defaultPipelineLayout = { VK_NULL_HANDLE };
	VkImage depthImage;
	VkDeviceMemory depthImageMemory;
	VkImageView depthImageview;
	std::vector<VkFramebuffer> swapChainFramebuffers;
	std::vector<VkCommandBuffer> commandBuffers;
	std::vector<VkBuffer> uniformBuffers;
	std::vector<VkDeviceMemory> uniformBuffersMemory;
	std::vector<void*> uniformBuffersMapped;
	const int MAX_FRAMES_IN_FLIGHT = 2;
	const int MAX_NUM_TEXTURE_BINDING = 8;
	uint32_t currentFrame = 0;
	std::vector<VkSemaphore> imageAvailableSemaphores;
	std::vector<VkSemaphore> renderFinishedSemaphores;
	std::vector<VkFence> inFlightFences;
	bool framebufferResized = false;

public:
	void Init();
	void Render();
	void Clean();
	static Renderer* GetInstance();
	static Renderer* GetInstance(GLFWwindow* window, RendererCustomFuncs* funcs);
	void UpdateUniformBuffer(uint32_t currentImage, Utils::UniformBufferObject& ubo);

#pragma region Getter Functions
	//Gettter Functions
	const VkPipeline GetPipeline() const { return  defaultPipeline; }
	const VkPipelineLayout GetPipelineLayout() const { return defaultPipelineLayout; }
	const VkRenderPass GetRenderPass() const { return defaultRenderpass; }
	const VkExtent2D GetSwapChainExtent() const { return swapChainExtent; }
	const VkDescriptorSet GetDescriptorSet(uint32_t currentFrame) const { return isInitialized ? descriptorSets[currentFrame] : VK_NULL_HANDLE; }
	const VkSampler GetDefaultSampler() const { return defaultSampler; }
	const VkBuffer GetUniformBuffer(uint32_t currentFrame) const { return uniformBuffers[currentFrame]; }
#pragma endregion

private:
	//block copy constructor, assignment opperation for singleton pattern. 
	Renderer(GLFWwindow* wd, RendererCustomFuncs* funcs);
	Renderer& operator=(const Renderer& rhs) = delete;
	Renderer(const Renderer& rhs) = delete;
	~Renderer() { 
		Clean();
	};

	void CreateVKinstance();
	void SetupDebugMessenger();
	void CreateSurface();
	void PickFirstPhysicalDevice();
	void CreateLogicalDevice();
	void CreateSwapChain();
	void CreateDefaultDescriptorSetLayout();
	void CreateUniforBuffers();
	void CreateDescriptorPool();
	void CreateDescriptorSets();
	void CreateImageViews();
	void CreateDepthResources();
	void CreateFramebuffers();
	void CreateCommandPool();
	void CreateCommandBuffers();
	void CreateSyncObject();
	void CreateDefaultSampler();

	bool checkDeviceExtensionSupport(VkPhysicalDevice device);
	bool IsDeviceSuitable(VkPhysicalDevice device);
	VkSurfaceFormatKHR ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
	VkPresentModeKHR ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
	VkExtent2D ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);
	void CleanUpSwapChain();
	void RecreateSwapChain();
private:
	static void FramebufferResizeCallback(GLFWwindow* window, int width, int height);
};