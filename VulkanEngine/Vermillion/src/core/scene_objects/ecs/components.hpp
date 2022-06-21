#pragma once

#include "utils/types.hpp"

// the underlying type for the component flags
using ComponentFlagType = unsigned char;
enum class ComponentFlagBits : ComponentFlagType;
using ComponentFlags = vk::Flags<ComponentFlagBits>;
enum class ComponentFlagBits : ComponentFlagType
{
	Transform = 1 << 0,
	Camera = 1 << 1,
	TestComponent = 1 << 2
};

struct Transform
{
	float3 position = float3(0.0f, 0.0f, 0.0f);
	Quaternion rotation = glm::identity<Quaternion>();
	//float3 scale = float3(1.0f, 1.0f, 1.0f);
};

// TEMPORARY
namespace ecs_safe
{
	struct Camera
	{
		float aspectRatio = 1280.0f / 720.0f;
		float fov = 45.0f;
		float near = 0.1f, far = 1000.0f;
	};
}