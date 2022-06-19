#pragma once

#include "headers/lighting_pass.vs.hpp"
#include "headers/lighting_pass.ps.hpp"
#include "headers/geometry_pass.vs.hpp"
#include "headers/geometry_pass.ps.hpp"
#include "headers/swapchain_write.vs.hpp"
#include "headers/swapchain_write.ps.hpp"

struct ShaderData { const unsigned char* pData; size_t size; };
struct ShaderPack { ShaderData vs, ps; };
const ShaderPack geometryPass = { { geometry_pass_vs, sizeof(geometry_pass_vs) }, { geometry_pass_ps, sizeof(geometry_pass_ps) } };
const ShaderPack lightingPass = { { lighting_pass_vs, sizeof(lighting_pass_vs) }, { lighting_pass_ps, sizeof(lighting_pass_ps) } };
const ShaderPack swapchainWrite = { { swapchain_write_vs, sizeof(swapchain_write_vs) }, { swapchain_write_ps, sizeof(swapchain_write_ps) } };

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