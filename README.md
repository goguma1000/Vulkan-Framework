# Vulkan Renderer Framework
## 프로젝트 개요
Vulkan API를 이용한 렌더링 프레임워크 개발</br>
렌더링 관련 학습 시, 매번 초기 설정에 많은 시간을 소모하지 않고,</br> 
렌더링에만 집중할 수 있도록 하기 위해 개발한 프레임워크.</br>

### 현재 구현 된 Features

* Model loading using Assimp
* Texture maping
* Depth test
* Mipmap generate

### 구현 및 수정할 Feature
* Descriptor set 구조 개선
* Push constant
* Buffer 할당 개선
* Texture 생성
* 3D texture
* Camera class
</br>. . .

## Demo
![Alt text](https://github.com/goguma1000/Vulkan-Framework/blob/main/Readme_source/DemoIMG.PNG)
## Description
### main
main함수는 'VulkanRenderer.cpp'파일에 존재한다.</br>
~~~c++
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
	GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "DongHo Engine",nullptr, nullptr);
	//set custom function
	RendererCustomFuncs funcs;
	funcs.checkSuitableDeviceFunc = isDeviceSuitable;
	funcs.setPhysicalDeviceFeaturesFunc = SetPhysicalDeviceFeatures;
	funcs.checkSwapSurfaceFormatFunc = CheckSwapSurfaceSupport;
	funcs.checkSwapPresentModeFunc = CheckSwapPresentMode;
	funcs.renderFunc = drawFunc;
	Renderer* renderer = Renderer::GetInstance(window, &funcs);
    . . .
}
~~~
위 코드와 같이 Renderer의 custome function들을 설정할 수 있다.</br>
funcs.renderFunc을 제외한 나머지 항목들은 renderer class를 Init할 때 사용하므로</br>
GetInstance 함수를 호출하기 전에 설정해야 한다.</br>

**관련 코드 링크 :**</br>
[VulkanRenderer.cpp](https://github.com/goguma1000/Vulkan-Framework/blob/main/VulkanRenderer/VulkanRenderer/VulkanRenderer.cpp)

### Render Function
main함수의 render Loop는 다음과 같다.
~~~c++
int main()
{
	. . .
	while (!glfwWindowShouldClose(window)) {
		glfwPollEvents();
		renderer->Render();
	}
	. . .
}
~~~

~~~c++
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
~~~
Render함수에서는 화면에 그릴 swapchain image를 얻고, 이전에 설정한 render function을 호출한 후, </br>
record된 commandBuffer를 graphics queue에 제출하고, 그려진 image를 present queue에 제출하여 화면에 그린다.</br>

**관련 코드 링크 :**</br>
[Renderer.h](https://github.com/goguma1000/Vulkan-Framework/blob/main/VulkanRenderer/VulkanRenderer/Renderer.h)</br>
[Renderer.cpp](https://github.com/goguma1000/Vulkan-Framework/blob/main/VulkanRenderer/VulkanRenderer/Renderer.cpp)


### model load
~~~c++
class Model {
public:
	Model() { };
	Model(const Renderer* renderer, char* fn) {
		LoadModel(renderer, fn);
	}
	std::vector<Mesh> meshes;
	void Draw(VkCommandBuffer commandBuffer);
	void LoadModel(const Renderer* renderer, const std::string& fn);
	VkImageView GetTextureView(int idx) { return texture_loaded[idx].textureImageView; }
private:
	std::vector<Texture> texture_loaded;
private:
	void ProcessNode(const Renderer* renderer, aiNode* node, const aiScene* scene, const std::string& path);
	Mesh ProcessMesh(const Renderer* renderer, aiMesh* mesh, const aiScene* scene, const std::string& path);
	int TestLoadMaterialTexture(const Renderer* renderer, aiMaterial * mat, const std::string& path, bool sRGB, bool genMipmap = true);
};

~~~
Model class는 model에 사용된 texture와 mesh를 가지고 있다.</br>
LoadModel 함수를 통해서 model및 texture를 load할 수 있다.
~~~c++
// when you load 3d model
Model model;
model.LoadModel(renderer, filePath);
~~~ 
LoadModel 함수를 호출하려면 위와 같이 Model 변수를 미리 선언하고, 함수를 호출하면 된다.</br>
parameter로 renderer instance와 파일의 경로를 넣어주면 된다.</br>
(현재 renderer가 singleton pattern을 사용하고 있기 때문에 추후 함수 안에서 renderer instance를 얻을 수 있도록 수정할 예정.)
</br></br>
Mesh class는 다음과 같다.
~~~c++
class Mesh {
public:
	Mesh(std::vector<Vertex> _vertices, std::vector<unsigned int> _indices, Material _material) :vertices(_vertices), indices(_indices), material(_material) {
		VkDeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();
		CreateBuffer(vertices.data(), bufferSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,vertexBuffer, vertexBufferMemory, "vertexBuffer");
		bufferSize = sizeof(indices[0]) * indices.size();
		CreateBuffer(indices.data(), bufferSize, VK_BUFFER_USAGE_INDEX_BUFFER_BIT,indexBuffer, indexBufferMemory, "indexBuffer");
	}
	void Draw(VkCommandBuffer commandBuffer);
public:
	Material material;
private:
	std::vector<Vertex>			vertices;
	std::vector<unsigned int>	indices;
	VkBuffer vertexBuffer = VK_NULL_HANDLE;
	VkDeviceMemory vertexBufferMemory = VK_NULL_HANDLE;
	VkBuffer indexBuffer = VK_NULL_HANDLE;
	VkDeviceMemory indexBufferMemory = VK_NULL_HANDLE;

    . . .
};
~~~
Mesh 는 material, vertex data, index data, 그리고 그들을 담은 buffer가 존재한다.</br>
material에는 load된 texture의 index를 가지고 있다.</br>
</br>
Texture structure는 다음과 같다.
~~~c++
struct Texture{
	uint32_t mipLevels = 1;
	VkImage textureImage = VK_NULL_HANDLE;
	VkImageView textureImageView = VK_NULL_HANDLE;
	VkDeviceMemory textureImageMemory = VK_NULL_HANDLE;
	string path = "";
public:
	Texture(const string& _path) :path(_path) {};

	void create(const string& fn, bool sRGB = false, bool isHdr = false, bool genMipmap = true, VkImageTiling tiling = VK_IMAGE_TILING_OPTIMAL) {
		Renderer* renderer = Renderer::GetInstance();
		if (renderer == nullptr) {
			std::cout << "renderer instance is nullptr! please create renderer instance  calling GetInstance(GlfwWindow, rendererCustomFuncs)!";
			return;
		}
		void* buf = nullptr;
		int width = 0, height = 0, nChannels = 0;
		//4채널이미지가 아니면 4채널로 만들어 줘야함 1채널과 3채널은 STBI_rgb_alpha를 쓰면 되는데 2채널은 따로 추가해줘야함
		stbi_set_flip_vertically_on_load(true);
		if (isHdr) {
			buf = (float*)stbi_loadf(fn.c_str(), &width, &height, &nChannels, STBI_rgb_alpha);
		}
		else {
			buf = (unsigned char*)stbi_load(fn.c_str(), &width, &height, &nChannels, STBI_rgb_alpha);
		}

		if (buf) {
			cout << fn << " image load success! width : " << width << " height : " << height << " channels : " << nChannels << std::endl;
		}
		else {
			throw std::runtime_error("failed to load texture image!");
		}
		. . . 
        //create texture image and mipmap
        //create texture imageview
	}
~~~
Texture struct는 texture의 miplevel과 texture image 및 view를 갖고있다.</br>
~~~c++
Texture texture(filePath);;
texture.create(filePath, ...);
~~~
Texture loading은 위와 같이 texture 변수를 선언하고 create함수를 호출하면 된다.</br>
(Texture의 constructor에 이미 filePath가 들어가므로, 추후에 create함수에서 filePath parameter를 제거하고</br>
 texture를 생성을 지원할 수 있도록 수정할 예정.)</br>

**관련 코드 링크 :**</br>
[Model.hpp](https://github.com/goguma1000/Vulkan-Framework/blob/main/VulkanRenderer/VulkanRenderer/Model/Model.hpp)</br>
[Model.cpp](https://github.com/goguma1000/Vulkan-Framework/blob/main/VulkanRenderer/VulkanRenderer/Model/Model.cpp)</br>

[Mesh.hpp](https://github.com/goguma1000/Vulkan-Framework/blob/main/VulkanRenderer/VulkanRenderer/Model/Mesh.hpp)</br>
[Mesh.cpp](https://github.com/goguma1000/Vulkan-Framework/blob/main/VulkanRenderer/VulkanRenderer/Model/Mesh.cpp)</br>

[Texture.hpp](https://github.com/goguma1000/Vulkan-Framework/blob/main/VulkanRenderer/VulkanRenderer/Model/Texture.hpp)</br>


### draw model
model을 draw할 때는 custome render function에서 다음과 같이 코드를 작성하면 된다.</br>
~~~c++
void drawFunc(VkCommandBuffer commandBuffer, VkFramebuffer framebuffer, uint32_t currentFrame) {
	. . .
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
	. . .
~~~
for loop에서 model의 각 mesh를 draw해주면 된다.</br>
texture를 binding하기 위해 위와 같이 for loop 안에서 descriptorWrite를 작성하면 된다.</br>
descriptorWrite를 작성하고, descriptorSet을 update하고 bind하고 mesh의 draw함수를 호출하면 된다.</br>
(Vulkan에서 한 번 update된 descriptor를 다시 update하는 것은 유효한 사용법이 아니라 validation layer에 걸린다.</br>
추후에 mesh별로 원하는 texture를 binding하면서 올바르게 사용하는 방법을 찾아서 수정할 예정.)</br>


