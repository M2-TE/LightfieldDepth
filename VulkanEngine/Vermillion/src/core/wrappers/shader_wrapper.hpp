#pragma once

#include "shaders.hpp"

vk::ShaderModule create_shader_module(DeviceWrapper& deviceWrapper, const unsigned char* data, size_t size)
{
	vk::ShaderModuleCreateInfo shaderInfo = vk::ShaderModuleCreateInfo()
		.setCodeSize(size)
		.setPCode(reinterpret_cast<const uint32_t*>(data)); // data alignment?
	return deviceWrapper.logicalDevice.createShaderModule(shaderInfo);
}
vk::ShaderModule create_shader_module(DeviceWrapper& deviceWrapper, ShaderData shader)
{
	vk::ShaderModuleCreateInfo shaderInfo = vk::ShaderModuleCreateInfo()
		.setCodeSize(shader.size)
		.setPCode(reinterpret_cast<const uint32_t*>(shader.pData)); // data alignment?
	return deviceWrapper.logicalDevice.createShaderModule(shaderInfo);
}