#pragma once

template<class T>
class UniformBufferWrapper
{
public:
	UniformBufferWrapper() = default;
	~UniformBufferWrapper() = default;
	ROF_COPY_MOVE_DELETE(UniformBufferWrapper)

public:
	void bind()
	{
		// TODO when encapsulating descriptor sets
	}

	void update(size_t iBuffer)
	{
		// already mapped, so just copy over
		memcpy(allocInfos[iBuffer].pMappedData, &data, sizeof(T));
	}
	void allocate(vma::Allocator& allocator, size_t nBuffers)
	{
		size_t bufferSize = sizeof(T);

		// buffer
		vk::BufferCreateInfo bufferInfo = vk::BufferCreateInfo()
			.setSize(bufferSize)
			.setUsage(vk::BufferUsageFlagBits::eUniformBuffer);

		vma::AllocationCreateInfo allocCreateInfo = vma::AllocationCreateInfo()
			.setUsage(vma::MemoryUsage::eAuto)
			.setFlags(vma::AllocationCreateFlagBits::eHostAccessSequentialWrite | vma::AllocationCreateFlagBits::eMapped);

		buffers.resize(nBuffers);
		allocInfos.resize(nBuffers);
		for (size_t i = 0; i < nBuffers; i++) {
			buffers[i] = allocator.createBuffer(bufferInfo, allocCreateInfo, allocInfos[i]);
			allocator.setAllocationName(buffers[i].second, "Uniform Buffer");
		}
	}
	void deallocate(vma::Allocator& allocator)
	{
		for (size_t i = 0; i < buffers.size(); i++) {
			allocator.destroyBuffer(buffers[i].first, buffers[i].second);
		}
	}

	vk::Buffer& get_buffer(size_t iBuffer)
	{
		return buffers[iBuffer].first;
	}
	size_t get_buffer_size()
	{
		return sizeof(T);
	}
public:
	T data;

private:
	std::vector<std::pair<vk::Buffer, vma::Allocation>> buffers;
	std::vector<vma::AllocationInfo> allocInfos;
};