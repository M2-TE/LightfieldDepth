#pragma once

typedef glm::vec<4, glm::f32, glm::packed_highp> float4;
typedef uint32_t Index;

#include "vertex.hpp"

class IndexedGeometry
{
public:
	IndexedGeometry() = default;
	~IndexedGeometry() = default;
	ROF_COPY_MOVE_DELETE(IndexedGeometry)

public:
	void draw(vk::CommandBuffer& commandBuffer) // can make bindless by storing the memory offsets in a manager or something?
	{
		vk::DeviceSize offsets[] = { 0 };
		commandBuffer.bindVertexBuffers(0, 1, &buffer.first, offsets);
		commandBuffer.bindIndexBuffer(buffer.first, sizeof(Vertex) * vertices.size(), vk::IndexType::eUint32);
		commandBuffer.drawIndexed(indices.size(), 1, 0, 0, 0);
	}

	void allocate(vma::Allocator& allocator, vk::CommandPool& transientCommandPool, DeviceWrapper& deviceWrapper)
	{
		size_t vertexSize = vertices.size() * sizeof(Vertex);
		size_t indexSize = indices.size() * sizeof(Index);
		size_t bufferSize = vertexSize + indexSize;

		// buffer
		vk::BufferCreateInfo bufferInfo = vk::BufferCreateInfo()
			.setSize(bufferSize)
			.setUsage(vk::BufferUsageFlagBits::eTransferDst |
				vk::BufferUsageFlagBits::eVertexBuffer |
				vk::BufferUsageFlagBits::eIndexBuffer);
		vma::AllocationCreateInfo allocCreateInfo = vma::AllocationCreateInfo()
			.setUsage(vma::MemoryUsage::eAutoPreferDevice);
		buffer = allocator.createBuffer(bufferInfo, allocCreateInfo);
		allocator.setAllocationName(buffer.second, "Indexed Geometry Buffer");

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
		memcpy(allocInfo.pMappedData, vertices.data(), vertexSize);
		memcpy(static_cast<char*>(allocInfo.pMappedData) + vertexSize, indices.data(), indexSize);

		// memory transfer
		{
			vk::CommandBufferAllocateInfo allocInfo = vk::CommandBufferAllocateInfo()
				.setLevel(vk::CommandBufferLevel::ePrimary)
				.setCommandPool(transientCommandPool)
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
			commandBuffer.copyBuffer(stagingBuffer.first, buffer.first, copyRegion);
			commandBuffer.end();

			vk::SubmitInfo submitInfo = vk::SubmitInfo()
				.setCommandBufferCount(1)
				.setPCommandBuffers(&commandBuffer);
			deviceWrapper.queue.submit(submitInfo);
			deviceWrapper.queue.waitIdle(); // TODO: change this to wait on a fence instead (upon queue submit) so multiple memory transfers would be possible

			// free command buffer directly after use
			deviceWrapper.logicalDevice.freeCommandBuffers(transientCommandPool, commandBuffer);
		}

		allocator.destroyBuffer(stagingBuffer.first, stagingBuffer.second);

	}
	void deallocate(vma::Allocator& allocator)
	{
		allocator.destroyBuffer(buffer.first, buffer.second);
	}

private:
	// TODO: allocate both vertex and index buffer to the same buffer, access via offsets
	std::pair<vk::Buffer, vma::Allocation> buffer;

	std::vector<Vertex> vertices = {
		Vertex(float4(-0.5f, -0.5f, 0.0f, 1.0f), float4(1.0f, 0.0f, 0.0f, 1.0f)),
		Vertex(float4( 0.5f, -0.5f, 0.0f, 1.0f), float4(0.0f, 1.0f, 0.0f, 1.0f)),
		Vertex(float4( 0.5f,  0.5f, 0.0f, 1.0f), float4(0.0f, 0.0f, 1.0f, 1.0f)),
		Vertex(float4(-0.5f,  0.5f, 0.0f, 1.0f), float4(1.0f, 1.0f, 0.0f, 1.0f))
	};
	std::vector<Index> indices = { 0, 1, 2, 2, 3, 0 };
};