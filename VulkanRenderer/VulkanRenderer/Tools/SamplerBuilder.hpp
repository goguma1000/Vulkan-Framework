#pragma once

#ifndef SAMPLERBUILDER_HPP
#define SAMPLERBUILDER_HPP
#define GLFW_INCLUDE_VULKAN
#include<GLFW/glfw3.h>
#include <stdexcept>
namespace SamplerBuilder {
	void CreateSampler(VkDevice device, VkSampler& out, const VkSamplerCreateInfo& samplerInfo) {
		if (vkCreateSampler(device, &samplerInfo, nullptr, &out) != VK_SUCCESS) {
			throw std::runtime_error("failed to create texture sampler!");
		}
	}
	VkSamplerCreateInfo InitSamplerCreateInfo(
		float _maxLod = 0.0f, float _minLod = 0.0f, float _mipLodBias = 0.0f, VkSamplerMipmapMode _mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
		VkBool32 _anisotropyEnable = VK_FALSE, float _maxAnisotropy = 1.0f, VkBool32 _compareEnable = VK_FALSE, VkCompareOp _compareOP = VK_COMPARE_OP_ALWAYS, 
		VkFilter _magFilter = VK_FILTER_LINEAR, VkFilter _minFilter = VK_FILTER_LINEAR,
		VkSamplerAddressMode _addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT, VkSamplerAddressMode _addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT, VkSamplerAddressMode _addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT,
		VkBorderColor _borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK, VkBool32 _unnormalizeCoordinates = VK_FALSE
	) {
		
		VkSamplerCreateInfo samplerInfo{};
		samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		samplerInfo.magFilter = _magFilter;
		samplerInfo.minFilter = _minFilter;
		samplerInfo.addressModeU = _addressModeU;
		samplerInfo.addressModeV = _addressModeV;
		samplerInfo.addressModeW = _addressModeW;
		samplerInfo.anisotropyEnable = _anisotropyEnable;
		samplerInfo.maxAnisotropy = _maxAnisotropy;
		samplerInfo.borderColor = _borderColor;
		samplerInfo.unnormalizedCoordinates = _unnormalizeCoordinates;
		samplerInfo.compareEnable = _compareEnable;
		samplerInfo.compareOp = _compareOP;
		samplerInfo.mipmapMode = _mipmapMode;
		samplerInfo.mipLodBias = _mipLodBias;
		samplerInfo.minLod = _minLod;
		samplerInfo.maxLod = _maxLod; //device의 최대 miplevel지원값으로.

		return samplerInfo;
	}
}
#endif // !SAMPLERBUILDER_HPP
