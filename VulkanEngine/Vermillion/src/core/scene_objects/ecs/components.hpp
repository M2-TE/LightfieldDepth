#pragma once

#include "utils/types.hpp"
#include "vk_mem_alloc.hpp"
#include "buffers/ring_buffer.hpp"

// the underlying type for the component flags
using ComponentFlagType = unsigned char;
enum class ComponentFlagBits : ComponentFlagType;
using ComponentFlags = vk::Flags<ComponentFlagBits>; 
enum class ComponentFlagBits : ComponentFlagType
{
	eNone = 0,
	eTransform = 1 << 1,
	eTransformBufferDynamic = 1 << 2,
	eTransformBufferStatic = 1 << 3,
	eGeometry = 1 << 4,
	eCamera = 1 << 5,
};


struct Transform
{
	float3 position = float3(0.0f, 0.0f, 0.0f);
	Quaternion rotation = glm::identity<Quaternion>();
	float3 scale = float3(1.0f, 1.0f, 1.0f);

private:
	friend class System;
	// TODO: data that only systems may read (buffers etc)
};
struct TransformBufferDynamic
{
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
	vk::Buffer buffer;
	vma::Allocation alloc;
	vma::AllocationInfo allocInfo;
	vk::DescriptorSet descSet;
};

struct Geometry
{
	vk::Buffer buffer;
	vma::Allocation alloc;

	struct Vertex
	{
	public:
		Vertex(float4 pos, float4 col) : pos(pos), col(col) {}

	public:
		static vk::VertexInputBindingDescription get_binding_desc() {
			vk::VertexInputBindingDescription desc = vk::VertexInputBindingDescription()
				.setBinding(0)
				.setStride(sizeof(Vertex))
				.setInputRate(vk::VertexInputRate::eVertex);
			return desc;
		}
		static std::array<vk::VertexInputAttributeDescription, 2> get_attr_desc() {
			std::array<vk::VertexInputAttributeDescription, 2> desc;
			desc[0] = vk::VertexInputAttributeDescription()
				.setBinding(0)
				.setLocation(0)
				.setFormat(vk::Format::eR32G32B32A32Sfloat)
				.setOffset(offsetof(Vertex, pos));
			desc[1] = vk::VertexInputAttributeDescription()
				.setBinding(0)
				.setLocation(1)
				.setFormat(vk::Format::eR32G32B32A32Sfloat)
				.setOffset(offsetof(Vertex, col));
			return desc;
		}

	public:
		glm::vec<4, glm::f32, glm::packed_highp> pos;
		glm::vec<4, glm::f32, glm::packed_highp> col;
	};
	std::vector<Vertex> vertices = {
		Vertex(float4(-0.5f, -0.5f, 0.0f, 1.0f), float4(1.0f, 0.0f, 0.0f, 1.0f)),
		Vertex(float4(0.5f, -0.5f, 0.0f, 1.0f), float4(0.0f, 1.0f, 0.0f, 1.0f)),
		Vertex(float4(0.5f,  0.5f, 0.0f, 1.0f), float4(0.0f, 0.0f, 1.0f, 1.0f)),
		Vertex(float4(-0.5f,  0.5f, 0.0f, 1.0f), float4(1.0f, 1.0f, 0.0f, 1.0f)),

		Vertex(float4(-0.5f, -0.5f, 0.5f, 1.0f), float4(1.0f, 0.0f, 0.0f, 1.0f)),
		Vertex(float4(0.5f, -0.5f, 0.5f, 1.0f), float4(0.0f, 1.0f, 0.0f, 1.0f)),
		Vertex(float4(0.5f,  0.5f, 0.5f, 1.0f), float4(0.0f, 0.0f, 1.0f, 1.0f)),
		Vertex(float4(-0.5f,  0.5f, 0.5f, 1.0f), float4(1.0f, 1.0f, 0.0f, 1.0f))
	};
	std::vector<Index> indices = {
		0, 1, 2, 2, 3, 0,
		4, 5, 6, 6, 7, 4
	};
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


