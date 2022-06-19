#pragma once

#include "wrappers/uniform_buffer_wrapper.hpp"

struct ViewProjectionBuffer { float4x4 view, proj; };
class Camera
{
public:
	Camera() = default;
	~Camera() = default;
	ROF_COPY_MOVE_DELETE(Camera)

public:
	void init(DeviceWrapper& deviceWrapper, vma::Allocator& allocator, vk::DescriptorPool& descPool, uint32_t nMaxFrames)
	{
		viewProjBuffer.init(deviceWrapper, allocator, descPool, 1, vk::ShaderStageFlagBits::eVertex, nMaxFrames);
	}
	void destroy(DeviceWrapper& deviceWrapper, vma::Allocator& allocator)
	{
		viewProjBuffer.destroy(deviceWrapper, allocator);
	}
	void rebuild()
	{
		// TODO (on e.g. change of nFrames in flight)
	}

	void update_uniform_buffer(DeviceWrapper& deviceWrapper, vk::Extent2D& extent, uint32_t iCurrentFrame)
	{
		static auto startTime = std::chrono::high_resolution_clock::now();
		auto currentTime = std::chrono::high_resolution_clock::now();
		float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

		auto& ubo = viewProjBuffer.data;
		ubo.view = glm::lookAt(glm::vec3(0.0f, 2.0f, -2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
		ubo.proj = glm::perspective(glm::radians(45.0f), (float)extent.width / (float)extent.height, 0.1f, 10.0f);

		viewProjBuffer.update(iCurrentFrame);
	}

public:
	UniformBufferWrapper<ViewProjectionBuffer> viewProjBuffer;

private:
	float4 position;
	float4 rotation;
	float4 rotationEuler;
};