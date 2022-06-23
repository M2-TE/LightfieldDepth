#pragma once

#include "utils/types.hpp"
#include "vk_mem_alloc.hpp"
#include "buffers/ring_buffer.hpp"
#include "scene_objects/geometry/vertex.hpp"

#define Component_Flag(comp) typeid(comp).hash_code()

namespace Components
{
	// represents an array of components to index into using entity ids
	struct ComponentArrayBase {};
	template<class T> struct ComponentArray : public ComponentArrayBase { std::array<T, nMaxEntities> components; };

	struct Allocator {  };
	struct Deallocator {  };

	struct Transform
	{
		float3 position = float3(0.0f, 0.0f, 0.0f);
		Quaternion rotation = glm::identity<Quaternion>();
		float3 scale = float3(1.0f, 1.0f, 1.0f);
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
		// TODO: could split this up into more components to improve memory access
		// e.g.: have desc set in a separate buffer, such that many desc sets fit into a single cache line
		vk::Buffer buffer;
		vma::Allocation alloc;
		vma::AllocationInfo allocInfo;
		vk::DescriptorSet descSet;
	};

	struct Geometry
	{
		vk::Buffer buffer;
		vma::Allocation alloc;

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

	struct Camera
	{
		float aspectRatio = 1280.0f / 720.0f;
		float fov = 45.0f;
		float near = 0.1f, far = 1000.0f;
	};
}