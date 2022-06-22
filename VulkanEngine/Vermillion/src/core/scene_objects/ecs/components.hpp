#pragma once

#include "utils/types.hpp"
#include "vk_mem_alloc.hpp"
#include "buffers/ring_buffer.hpp"

// the underlying type for the component flags
using ComponentFlagType = unsigned char;
enum class ComponentFlagBits : ComponentFlagType;
using ComponentFlags = vk::Flags<ComponentFlagBits>;

struct Transform
{
	static constexpr ComponentFlagType sIndex = 0;

	float3 position = float3(0.0f, 0.0f, 0.0f);
	Quaternion rotation = glm::identity<Quaternion>();
	float3 scale = float3(1.0f, 1.0f, 1.0f);

private:
	friend class System;
	// TODO: data that only systems may read (buffers etc)
};
struct TransformBufferDynamic
{
	static constexpr ComponentFlagType sIndex = Transform::sIndex + 1;

	struct BufferFrame
	{
		vk::Buffer buffer;
		vma::Allocation alloc;
		vma::AllocationInfo allocInfo;
		vk::DescriptorSet descSet;
	};
	RingBuffer<BufferFrame> bufferFrames;
};
struct TransformBufferStatic
{
	static constexpr ComponentFlagType sIndex = TransformBufferDynamic::sIndex + 1;

	vk::Buffer buffer;
	vma::Allocation alloc;
	vma::AllocationInfo allocInfo;
	vk::DescriptorSet descSet;
};

struct Geometry
{
	static constexpr ComponentFlagType sIndex = TransformBufferStatic::sIndex + 1;
};

// TEMPORARY
namespace ecs_safe
{
	struct Camera
	{
		static constexpr ComponentFlagType sIndex = Geometry::sIndex + 1;

		float aspectRatio = 1280.0f / 720.0f;
		float fov = 45.0f;
		float near = 0.1f, far = 1000.0f;
	};
}

static constexpr ComponentFlagType nMaxComponents = ecs_safe::Camera::sIndex + 1;

enum class ComponentFlagBits : ComponentFlagType
{
	eNone = 0,
	eTransform = 1 << Transform::sIndex,
	eTransformBufferDynamic = 1 << TransformBufferDynamic::sIndex,
	eTransformBufferStatic = 1 << TransformBufferStatic::sIndex,
	eGeometry = 1 << Geometry::sIndex,
	eCamera = 1 << ecs_safe::Camera::sIndex,
};
