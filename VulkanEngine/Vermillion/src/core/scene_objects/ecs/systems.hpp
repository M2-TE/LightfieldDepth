#pragma once

//#include <core/scene_objects/ecs/components.hpp>
#include "scene_objects/geometry/vertex.hpp"

namespace Systems
{
#define EXECUTION_INPUT std::unordered_map<size_t, Components::ComponentArrayBase*>& allComponents, std::unordered_map<size_t, std::set<Entity>>& allEntities
#define EXECUTE(...) static inline void execute(EXECUTION_INPUT, __VA_ARGS__)
#define GET_COMPONENTS(name) static_cast<Components::ComponentArray<name>*>(allComponents[Component_Flag(name)])->components
#define GET_ENTITIES(name) allEntities[name().componentFlags]

	// TODO: pass entities into these funcs too, so the actual flags can get removed
	// maybe just pass the ECS& directly? that way theres more direct control

	struct Geometry
	{
		struct Allocator
		{
			const size_t componentFlags = Component_Flag(Components::Geometry) | Component_Flag(Components::Allocator);

			EXECUTE(DeviceWrapper& deviceWrapper, vma::Allocator& allocator, vk::CommandPool& commandPool)
			{
				auto& arr = GET_COMPONENTS(Components::Geometry);

				// can prolly put this into a macro
				auto& entities = GET_ENTITIES(Allocator);
				auto cur = entities.cbegin();
				auto end = entities.cend();
				for (; cur != end; cur++) {
					VMI_LOG("allocating " << *cur);
					allocate(arr[*cur], deviceWrapper, allocator, commandPool);
				}

				// clear the allocated objects so they only get allocated once
				GET_ENTITIES(Allocator).clear();
			}

		private:
			static inline void allocate(Components::Geometry& geometry, DeviceWrapper& deviceWrapper, vma::Allocator& allocator, vk::CommandPool& commandPool)
			{
				size_t vertexSize = geometry.vertices.size() * sizeof(Vertex);
				size_t indexSize = geometry.indices.size() * sizeof(Index);
				size_t bufferSize = vertexSize + indexSize;

				// buffer
				vk::BufferCreateInfo bufferInfo = vk::BufferCreateInfo()
					.setSize(bufferSize)
					.setUsage(vk::BufferUsageFlagBits::eTransferDst |
						vk::BufferUsageFlagBits::eVertexBuffer |
						vk::BufferUsageFlagBits::eIndexBuffer);
				vma::AllocationCreateInfo allocCreateInfo = vma::AllocationCreateInfo()
					.setUsage(vma::MemoryUsage::eAutoPreferDevice);
				allocator.createBuffer(&bufferInfo, &allocCreateInfo, &geometry.buffer, &geometry.alloc, nullptr);

				// staging buffer
				bufferInfo = vk::BufferCreateInfo()
					.setSize(bufferSize)
					.setUsage(vk::BufferUsageFlagBits::eTransferSrc);
				allocCreateInfo = vma::AllocationCreateInfo()
					.setUsage(vma::MemoryUsage::eAuto)
					.setFlags(vma::AllocationCreateFlagBits::eHostAccessSequentialWrite | vma::AllocationCreateFlagBits::eMapped);
				vma::AllocationInfo allocInfo;
				auto stagingBuffer = allocator.createBuffer(bufferInfo, allocCreateInfo, allocInfo);

				// already mapped, so just copy over
				memcpy(allocInfo.pMappedData, geometry.vertices.data(), vertexSize);
				memcpy(static_cast<char*>(allocInfo.pMappedData) + vertexSize, geometry.indices.data(), indexSize);

				// memory transfer
				{
					vk::CommandBufferAllocateInfo allocInfo = vk::CommandBufferAllocateInfo()
						.setLevel(vk::CommandBufferLevel::ePrimary)
						.setCommandPool(commandPool)
						.setCommandBufferCount(1);

					vk::CommandBuffer commandBuffer;
					auto res = deviceWrapper.logicalDevice.allocateCommandBuffers(&allocInfo, &commandBuffer);

					// begin recording to temporary command buffer
					vk::CommandBufferBeginInfo beginInfo = vk::CommandBufferBeginInfo()
						.setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
					commandBuffer.begin(beginInfo);

					vk::BufferCopy copyRegion = vk::BufferCopy()
						.setSrcOffset(0)
						.setDstOffset(0)
						.setSize(bufferSize);
					commandBuffer.copyBuffer(stagingBuffer.first, geometry.buffer, copyRegion);
					commandBuffer.end();

					vk::SubmitInfo submitInfo = vk::SubmitInfo()
						.setCommandBufferCount(1)
						.setPCommandBuffers(&commandBuffer);
					deviceWrapper.queue.submit(submitInfo);
					deviceWrapper.queue.waitIdle(); // TODO: change this to wait on a fence instead (upon queue submit) so multiple memory transfers would be possible

					// free command buffer directly after use
					deviceWrapper.logicalDevice.freeCommandBuffers(commandPool, commandBuffer);
				}

				allocator.destroyBuffer(stagingBuffer.first, stagingBuffer.second);
			}
		};

		struct Deallocator
		{
			const size_t componentFlags = Component_Flag(Components::Geometry) | Component_Flag(Components::Deallocator);

			EXECUTE(vma::Allocator& allocator)
			{
				auto& arr = GET_COMPONENTS(Components::Geometry);

				auto& entities = GET_ENTITIES(Renderer);
				auto cur = entities.cbegin();
				auto end = entities.cend();
				for (; cur != end; cur++) {
					VMI_LOG("deallocating " << *cur);
					deallocate(arr[*cur], allocator);
				}

				// remove so they dont get deallocated/rendered again
				GET_ENTITIES(Deallocator).clear();
				GET_ENTITIES(Renderer).clear();
			}

		private:
			static inline void deallocate(Components::Geometry& geometry, vma::Allocator& allocator)
			{
				allocator.destroyBuffer(geometry.buffer, geometry.alloc);
			}
		};

		struct Renderer
		{
			const size_t componentFlags = Component_Flag(Components::Geometry);

			EXECUTE(vk::CommandBuffer& commandBuffer)
			{
				auto& arr = GET_COMPONENTS(Components::Geometry);

				auto& entities = GET_ENTITIES(Renderer);
				auto cur = entities.cbegin();
				auto end = entities.cend();
				for (; cur != end; cur++) render(arr[*cur], commandBuffer);
			}

		private:
			static inline void render(Components::Geometry& geometry, vk::CommandBuffer& commandBuffer)
			{
				vk::DeviceSize offsets[] = { 0 };
				commandBuffer.bindVertexBuffers(0, 1, &geometry.buffer, offsets);
				commandBuffer.bindIndexBuffer(geometry.buffer, sizeof(Vertex) * geometry.vertices.size(), vk::IndexType::eUint32);
				commandBuffer.drawIndexed((uint32_t)geometry.indices.size(), 1, 0, 0, 0);
			}
		};
	};
}