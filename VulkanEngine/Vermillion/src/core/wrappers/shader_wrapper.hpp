#pragma once

#include "shaders.hpp"

vk::ShaderModule create_shader_module(DeviceWrapper& deviceWrapper, const unsigned char* data, size_t size)
{
	vk::ShaderModuleCreateInfo shaderInfo = vk::ShaderModuleCreateInfo()
		.setCodeSize(size)
		.setPCode(reinterpret_cast<const uint32_t*>(data)); // data alignment?
	return deviceWrapper.logicalDevice.createShaderModule(shaderInfo);
}