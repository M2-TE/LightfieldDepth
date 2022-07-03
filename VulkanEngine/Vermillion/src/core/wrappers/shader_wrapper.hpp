#pragma once

#include "./../shaders/lighting_pass_vs.hpp"
#include "./../shaders/lighting_pass_ps.hpp"
#include "./../shaders/geometry_pass_vs.hpp"
#include "./../shaders/geometry_pass_ps.hpp"
#include "./../shaders/swapchain_write_vs.hpp"
#include "./../shaders/swapchain_write_ps.hpp"
#include "./../shaders/lightfield_write_vs.hpp"
#include "./../shaders/lightfield_write_ps.hpp"
#include "./../shaders/lightfield_gradients_vs.hpp"
#include "./../shaders/lightfield_gradients_ps.hpp"
#include "./../shaders/lightfield_disparity_vs.hpp"
#include "./../shaders/lightfield_disparity_ps.hpp"

struct ShaderData { const unsigned char* pData; size_t size; };
struct ShaderPack { ShaderData vs, ps; };
const ShaderPack geometryPass = { { geometry_pass_vs, sizeof(geometry_pass_vs) }, { geometry_pass_ps, sizeof(geometry_pass_ps) } };
const ShaderPack lightingPass = { { lighting_pass_vs, sizeof(lighting_pass_vs) }, { lighting_pass_ps, sizeof(lighting_pass_ps) } };
const ShaderPack lightfieldWrite = { { lightfield_write_vs, sizeof(lightfield_write_vs) }, { lightfield_write_ps, sizeof(lightfield_write_ps) } };
const ShaderPack lightfieldGradients = { { lightfield_gradients_vs, sizeof(lightfield_gradients_vs) }, { lightfield_gradients_ps, sizeof(lightfield_gradients_ps) } };
const ShaderPack lightfieldDisparity = { { lightfield_disparity_vs, sizeof(lightfield_disparity_vs) }, { lightfield_disparity_ps, sizeof(lightfield_disparity_ps) } };
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