#pragma once

#include "utils/types.hpp"
#include "vk_mem_alloc.hpp"
#include "buffers/ring_buffer.hpp"

namespace components
{
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
			vk::DescriptorSet descSet;
			vma::Allocation alloc;
			vma::AllocationInfo allocInfo;
		};
		RingBuffer<BufferFrame> bufferFrames;
	};
	struct TransformBufferStatic
	{
		// TODO: could split this up into more components to improve memory access
		// e.g.: have desc set in a separate buffer, such that many desc sets fit into a single cache line
		vk::Buffer buffer;
		vk::DescriptorSet descSet;
		vma::Allocation alloc;
		vma::AllocationInfo allocInfo;
	};
}

namespace systems
{
	struct Transform
	{
		static void allocate()
		{

		}

		static void deallocate()
		{

		}

		static void update_dynamic()
		{
			// TODO
		}

		static void update_static()
		{
			// TODO
		}
	};
}