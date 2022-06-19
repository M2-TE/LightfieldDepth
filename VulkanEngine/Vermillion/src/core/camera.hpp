#pragma once

#include "uniform_buffer.hpp"
#include "wrappers/swapchain_wrapper.hpp"

// why were these defined in the first place.. tf?
#undef near
#undef far

struct ViewProjectionBuffer { float4x4 view, proj, viewProj; };
class Camera
{
public:
	Camera() = default;
	~Camera() = default;
	ROF_COPY_MOVE_DELETE(Camera)

public:
	void init(DeviceWrapper& deviceWrapper, vma::Allocator& allocator, vk::DescriptorPool& descPool, SwapchainWrapper& swapchainWrapper)
	{
		aspectRatio = (float)swapchainWrapper.extent.width / (float)swapchainWrapper.extent.height;

		auto& ubo = viewProjBuffer.data;
		// view matrix
		ubo.view = glm::mat4_cast(glm::inverse(rotation));
		ubo.view = glm::translate(ubo.view, -position);
		// projection matrix
		ubo.proj = glm::perspective(fov, aspectRatio, near, far);
		// combination
		ubo.viewProj = ubo.proj * ubo.view;

		BufferInfo info = {
			deviceWrapper,
			allocator,
			descPool,
			vk::ShaderStageFlagBits::eVertex,
			1, swapchainWrapper.images.size()
		};
		viewProjBuffer.init(info);
	}
	void destroy(DeviceWrapper& deviceWrapper, vma::Allocator& allocator)
	{
		viewProjBuffer.destroy(deviceWrapper, allocator);
	}
	void rebuild(DeviceWrapper& deviceWrapper, vma::Allocator& allocator, vk::DescriptorPool& descPool, SwapchainWrapper& swapchainWrapper)
	{
		destroy(deviceWrapper, allocator);
		init(deviceWrapper, allocator, descPool, swapchainWrapper);
	}

	void translate(float3 dir)
	{
		position += dir;
	}
	void rotate_euler(float3 rot)
	{
		rotation *= Quaternion(rot);
	}
	
	void update()
	{
		auto& ubo = viewProjBuffer.data;

		// view matrix
		ubo.view = glm::mat4_cast(glm::inverse(rotation));
		ubo.view = glm::translate(ubo.view, -position);

		// projection matrix
		ubo.proj = glm::perspective(fov, aspectRatio, near, far);

		// combination
		ubo.viewProj = ubo.proj * ubo.view;
		
		viewProjBuffer.update();
	}
	vk::DescriptorSet& get_desc_set()
	{
		return viewProjBuffer.get_desc_set();
	}
	vk::DescriptorSetLayout& get_desc_set_layout()
	{
		return viewProjBuffer.descSetLayout;
	}

private:
	UniformBufferDynamic<ViewProjectionBuffer> viewProjBuffer;

	// camera position in world space
	float3 position = { 0.0f, 0.0f, -5.0f };
	Quaternion rotation = glm::identity<Quaternion>() * glm::quat({ 0.0f, glm::radians(10.0f), 0.0f});

	// camera settings
	float aspectRatio = 1280.0f / 720.0f;
	float fov = 45.0f;
	float near = 0.1f, far = 1000.0f;
};