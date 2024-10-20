#define GLFW_INCLUDE_VULKAN
#include<GLFW/glfw3.h>
#include <iostream>
#include <array>
#include "Renderer.h"
#include "Tools/Utils.hpp"
#include <glm/gtc/matrix_transform.hpp>
#include "Model/Model.hpp"

const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;

Model model;
Utils::UniformBufferObject ubo{};

#pragma region Renderer custom function

bool isDeviceSuitable(VkPhysicalDevice device) {
	return true;
}

bool CheckSwapSurfaceSupport(const VkSurfaceFormatKHR& availableFormat) {
	if (availableFormat.format == VK_FORMAT_R8G8B8A8_SRGB && availableFormat.colorSpace == VK_COLORSPACE_SRGB_NONLINEAR_KHR)
		return true;
	else return false;
}
bool CheckSwapPresentMode(const VkPresentModeKHR& availablePresentMode) {
	if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) return true;
	else return false;
}
void SetPhysicalDeviceFeatures(VkPhysicalDeviceFeatures& deviceFeatures) {
	
}

void drawFunc(VkCommandBuffer commandBuffer, VkFramebuffer framebuffer, uint32_t currentFrame) {
	Renderer* renderer = Renderer::GetInstance();
	if (renderer == nullptr) return;

	VkCommandBufferBeginInfo beginInfo = Initializer::InitCommandBufferBeginInfo();
	if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
		throw std::runtime_error("failed  to begin recording command buffer!");
	}
	VkExtent2D swapChainExtent = renderer->GetSwapChainExtent();
	std::array<VkClearValue, 2> clearValues{};
	clearValues[0].color = { {0.0f, 0.0f, 0.0f, 1.0f} };
	clearValues[1].depthStencil = {1.0f, 0};
	VkRenderPassBeginInfo renderPassInfo = 
		Initializer::InitRenderPassBeginInfo(renderer->GetRenderPass(), framebuffer, { 0,0 }, swapChainExtent, static_cast<uint32_t>(clearValues.size()), clearValues.data());
	vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, renderer->GetPipeline());
	
	VkViewport viewport = Initializer::InitViewport(0.0f, 0.0f, static_cast<float>(swapChainExtent.width), static_cast<float>(swapChainExtent.height), 0.0f, 1.0f);
	vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
	
	VkRect2D scissor = Initializer::InitScissor({ 0,0 }, swapChainExtent);
	vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
	
	//Write here
	std::vector<VkWriteDescriptorSet> descriptorWrites;
	descriptorWrites.emplace_back();
	VkDescriptorBufferInfo bufferInfo = Initializer::InitDescriptorBufferInfo(renderer->GetUniformBuffer(currentFrame), sizeof(Utils::UniformBufferObject));
	descriptorWrites[0] = Initializer::InitWriteDescriptorSet(renderer->GetDescriptorSet(currentFrame), 0, 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, &bufferInfo);
	descriptorWrites.emplace_back();
	for (auto& mesh : model.meshes) {
		if (mesh.material.diffTexIdx >= 0) {
			int diffIdx = mesh.material.diffTexIdx;
			int binding = descriptorWrites.size() - 1;
			//mesh에서 texturebind함수 만들어서 
			VkDescriptorImageInfo imageInfo = Initializer::InitDescriptorImageInfo(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, model.GetTextureView(diffIdx), renderer->GetDefaultSampler());
			descriptorWrites[binding] = Initializer::InitWriteDescriptorSet(renderer->GetDescriptorSet(currentFrame), binding, 0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, nullptr, &imageInfo);
		}
		VkDescriptorSet descriptorSet = renderer->GetDescriptorSet(currentFrame);
		vkUpdateDescriptorSets(renderer->device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, renderer->GetPipelineLayout(), 0, 1, &descriptorSet, 0, nullptr);
		mesh.Draw(commandBuffer);
	}
	//
	
	vkCmdEndRenderPass(commandBuffer);
	if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
		throw std::runtime_error("failed to record command buffer!");
	}
}
#pragma endregion

int main()
{
	if (!glfwInit()) {
		printf("Fail glfwInit\n");
		exit(EXIT_FAILURE);
	}
	//init window
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API); //GLFW was originally designed to create an OpenGL context											  
												  //we need to tell it to not create an OpenGL context
	glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
	GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan Engine",nullptr, nullptr);
	//set custom function
	RendererCustomFuncs funcs;
	funcs.checkSuitableDeviceFunc = isDeviceSuitable;
	funcs.setPhysicalDeviceFeaturesFunc = SetPhysicalDeviceFeatures;
	funcs.checkSwapSurfaceFormatFunc = CheckSwapSurfaceSupport;
	funcs.checkSwapPresentModeFunc = CheckSwapPresentMode;
	funcs.renderFunc = drawFunc;
	Renderer* renderer = Renderer::GetInstance(window, &funcs);
	model.LoadModel(renderer, "Assets/Camera_01_4k.gltf/Camera_01_4k.gltf");
	ubo.model = glm::scale(glm::mat4(1.0f), glm::vec3(10.0f));
	ubo.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
	ubo.proj = glm::perspective(glm::radians(45.0f), WIDTH / (float)HEIGHT, 0.1f, 10.0f);
	ubo.proj[1][1] = -1;
	renderer->UpdateUniformBuffer(0, ubo);
	renderer->UpdateUniformBuffer(1, ubo);

	while (!glfwWindowShouldClose(window)) {
		glfwPollEvents();
		renderer->Render();
	}
	renderer->Clean();
	glfwDestroyWindow(window);
	glfwTerminate();
	return 0;
}

