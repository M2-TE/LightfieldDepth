#pragma once

#include "wrappers/uniform_buffer_wrapper.hpp"

class Camera
{
public:
	Camera() = default;
	~Camera() = default;
	ROF_COPY_MOVE_DELETE(Camera)

private:

private:
	struct ViewProjectionBuffer { glm::mat<4, 4, glm::f32, glm::packed_highp> view, proj; };
	UniformBufferWrapper<ViewProjectionBuffer> mvpBuffer;
	glm::vec<4, float, glm::packed_highp> position;
	glm::vec<4, float, glm::packed_highp> rotation;
	glm::vec<4, float, glm::packed_highp> rotationEuler;
};

// TODO: decouple swapchain images in flight and standard images in flight!