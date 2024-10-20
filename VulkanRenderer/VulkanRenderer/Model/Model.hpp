#pragma once
#ifndef MODEL_HPP
#define MODEL_HPP
#include "Mesh.hpp"
#include "Texture.hpp"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/GltfMaterial.h>
#include <vector>

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
#endif // !1