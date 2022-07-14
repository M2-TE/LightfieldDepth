#pragma once

#include "buffers/uniform_buffer.hpp"
#include "wrappers/swapchain_wrapper.hpp"

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

		BufferInfo info = {
			deviceWrapper,
			allocator,
			descPool,
			stageFlags, binding,
			swapchainWrapper.images.size()
		};
		viewProjBuffer.init(info);
		update();
	}
	void destroy(DeviceWrapper& deviceWrapper, vma::Allocator& allocator)
	{
		viewProjBuffer.destroy(deviceWrapper, allocator);
	}

	void handle_input(Input& input)
	{
		static auto startTime = std::chrono::high_resolution_clock::now();
		auto currentTime = std::chrono::high_resolution_clock::now();
		float dt = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();
		startTime = std::chrono::high_resolution_clock::now();

		// rotation
		if (input.mouseButtonsDown.count(3) || input.mouseButtonsDown.count(1)) {
			//if (!SDL_GetRelativeMouseMode()) SDL_SetRelativeMouseMode(SDL_TRUE);

			float mouseSpeed = .1f;
			float xRot = glm::radians(static_cast<float>(input.xMouseRel) * mouseSpeed);
			float yRot = glm::radians(static_cast<float>(input.yMouseRel) * mouseSpeed);
			rotationEuler += float3(-yRot, xRot, 0.0f);
		}
		else {
			//if (SDL_GetRelativeMouseMode()) SDL_SetRelativeMouseMode(SDL_FALSE);
		}

		float speed = dt * 2.0f;
		// forward/backward
		if (input.keysDown.count(SDLK_w)) translate(float3(0.0f, 0.0f, speed));
		else if (input.keysDown.count(SDLK_s)) translate(float3(0.0f, 0.0f, -speed));

		// left/right
		if (input.keysDown.count(SDLK_d)) translate(float3(speed, 0.0f, 0.0f));
		else if (input.keysDown.count(SDLK_a)) translate(float3(-speed, 0.0f, 0.0f));

		// up/down
		if (input.keysDown.count(SDLK_q)) translate(float3(0.0f, speed, 0.0f));
		else if (input.keysDown.count(SDLK_e)) translate(float3(0.0f, -speed, 0.0f));

	}

	void translate(float3 dir)
	{
		position += glm::quat(rotationEuler) * dir;
	}
	void rotate_euler(float3 rot)
	{
		rotationEuler += rot;
	}
	
	void update()
	{
		auto& ubo = viewProjBuffer.data;

		// view matrix
		ubo.view = glm::mat4_cast(glm::inverse(glm::quat(rotationEuler)));
		ubo.view = glm::translate(ubo.view, -position);

		// projection matrix
		ubo.proj = glm::perspective(fov, aspectRatio, near, far);

		// combination
		ubo.viewProj = ubo.proj * ubo.view;
		
		viewProjBuffer.write_buffer();
	}
	vk::DescriptorSet& get_desc_set()
	{
		return viewProjBuffer.get_desc_set();
	}
	static vk::DescriptorSetLayout get_temp_desc_set_layout(DeviceWrapper& deviceWrapper)
	{
		return UniformBufferDynamic<ViewProjection>::get_temp_desc_set_layout(deviceWrapper, binding, stageFlags);
	}

private:
	static constexpr uint32_t binding = 1;
	static constexpr vk::ShaderStageFlags stageFlags = vk::ShaderStageFlagBits::eVertex;
	struct ViewProjection { float4x4 view, proj, viewProj; };
	UniformBufferDynamic<ViewProjection> viewProjBuffer;

	// camera position in world space
	float3 position = { 0.0f, 0.0f, -5.0f };
	float3 rotationEuler;

	// camera settings
	float aspectRatio = 1280.0f / 720.0f;
	float fov = 45.0f;
	float near = 0.1f, far = 1000.0f;
};