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


	void allocate(vma::Allocator& allocator, vk::Queue& transferQueue)
	{
		// buffer
		size_t bufferSize = vertices.size() * sizeof(Vertex);
		vk::BufferCreateInfo bufferInfo = vk::BufferCreateInfo()
			.setSize(bufferSize)
			.setUsage(vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eTransferDst);
		vma::AllocationCreateInfo allocCreateInfo = vma::AllocationCreateInfo()
			.setUsage(vma::MemoryUsage::eAutoPreferDevice);
		vertexBuffer = allocator.createBuffer(bufferInfo, allocCreateInfo);

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
		memcpy(allocInfo.pMappedData, &vertices, bufferSize);
		// TODO: memory transfer

		allocator.destroyBuffer(stagingBuffer.first, stagingBuffer.second);

	}
	void deallocate(vma::Allocator& allocator)
	{
		allocator.destroyBuffer(vertexBuffer.first, vertexBuffer.second);
	}

private:
	// TODO: allocate both vertex and index buffer to the same buffer, access via offsets
	std::pair<vk::Buffer, vma::Allocation> vertexBuffer;

	std::vector<Vertex> vertices = {
		Vertex(float4(-0.5f, -0.5f, 0.0f, 1.0f), float4(1.0f, 0.0f, 0.0f, 1.0f)),
		Vertex(float4( 0.5f, -0.5f, 0.0f, 1.0f), float4(0.0f, 1.0f, 0.0f, 1.0f)),
		Vertex(float4( 0.5f,  0.5f, 0.0f, 1.0f), float4(0.0f, 0.0f, 1.0f, 1.0f)),
		Vertex(float4(-0.5f,  0.5f, 0.0f, 1.0f), float4(1.0f, 1.0f, 0.0f, 1.0f))
	};
	std::vector<Index> indices = { 0, 1, 2, 2, 3, 0 };
};