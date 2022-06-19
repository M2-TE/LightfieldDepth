#pragma once

#include "ring_buffer.hpp"

struct BufferInfo
{
	DeviceWrapper& deviceWrapper;
	vma::Allocator& allocator;
	vk::DescriptorPool& descPool;
	vk::ShaderStageFlags stageFlags; 
	uint32_t binding;
	size_t nBuffers = 1;
};

template<class T>
class UniformBufferBase
{
protected:
	UniformBufferBase() = default;
	~UniformBufferBase() = default;
	ROF_COPY_MOVE_DELETE(UniformBufferBase)

public:
	void init(BufferInfo& info)
	{
		create_buffer(info);
		create_descriptor_set_layout(info);
		create_descriptor_sets(info);
	}
	void destroy(DeviceWrapper& deviceWrapper, vma::Allocator& allocator)
	{
		deviceWrapper.logicalDevice.destroyDescriptorSetLayout(descSetLayout); 
		destroy_buffer(allocator);
	}
	virtual void write_buffer() = 0;

protected:
	virtual void destroy_buffer(vma::Allocator& allocator) = 0;
	virtual void create_buffer(BufferInfo& info) = 0;
	virtual void create_descriptor_sets(BufferInfo& info) = 0;

private:
	void create_descriptor_set_layout(BufferInfo& info)
	{
		vk::DescriptorSetLayoutBinding layoutBinding = vk::DescriptorSetLayoutBinding()
			.setBinding(info.binding)
			.setStageFlags(info.stageFlags)
			.setDescriptorType(vk::DescriptorType::eUniformBuffer)
			.setDescriptorCount(1)
			.setPImmutableSamplers(nullptr);

		// create descriptor set layout from all the bindings
		vk::DescriptorSetLayoutCreateInfo createInfo = vk::DescriptorSetLayoutCreateInfo()
			.setBindingCount(1)
			.setBindings(layoutBinding);
		descSetLayout = info.deviceWrapper.logicalDevice.createDescriptorSetLayout(createInfo);
	}

public:
	T data;
	vk::DescriptorSetLayout descSetLayout;
};

template<class T>
class UniformBufferStatic : public UniformBufferBase<T>
{
public:
	UniformBufferStatic() = default;
	~UniformBufferStatic() = default;
	ROF_COPY_MOVE_DELETE(UniformBufferStatic)

public:

private:
	std::pair<vk::Buffer, vma::Allocation> buffer;
	vk::DescriptorSet descSet;
};

template<class T>
class UniformBufferDynamic : public UniformBufferBase<T>
{
public:
	UniformBufferDynamic() = default;
	~UniformBufferDynamic() = default;
	ROF_COPY_MOVE_DELETE(UniformBufferDynamic)

public:
	// init()
	// destroy()
	vk::DescriptorSet& get_desc_set()
	{
		return bufferFrames.get_current().descSet;
	}
	void write_buffer() override
	{
		// already mapped, so just copy over
		// additionally, advance frame by one, so the next free buffer frame gets written to
		memcpy(bufferFrames.get_next().allocInfo.pMappedData, &(UniformBufferBase<T>::data), sizeof(T));
	}

private:
	void destroy_buffer(vma::Allocator& allocator) override
	{
		bufferFrames.destroy(allocator);
	}
	void create_buffer(BufferInfo& info) override
	{
		vk::BufferCreateInfo bufferInfo = vk::BufferCreateInfo()
			.setSize(sizeof(T))
			.setUsage(vk::BufferUsageFlagBits::eUniformBuffer);

		vma::AllocationCreateInfo allocCreateInfo = vma::AllocationCreateInfo()
			.setUsage(vma::MemoryUsage::eAuto)
			.setFlags(vma::AllocationCreateFlagBits::eHostAccessSequentialWrite | vma::AllocationCreateFlagBits::eMapped);

		bufferFrames.set_size(info.nBuffers);
		for (size_t i = 0; i < info.nBuffers; i++) {
			auto& frame = bufferFrames[i];
			info.allocator.createBuffer(&bufferInfo, &allocCreateInfo, &frame.buffer, &frame.alloc, &frame.allocInfo);
		}
	}
	void create_descriptor_sets(BufferInfo& info) override
	{
		// duplicate layout to have one for each frame in flight
		std::vector<vk::DescriptorSetLayout> layouts(info.nBuffers, UniformBufferBase<T>::descSetLayout);

		// allocate the descriptor sets using descriptor pool
		vk::DescriptorSetAllocateInfo allocInfo = vk::DescriptorSetAllocateInfo()
			.setDescriptorPool(info.descPool)
			.setSetLayouts(layouts);
		auto descSetArr = info.deviceWrapper.logicalDevice.allocateDescriptorSets(allocInfo);
		for (size_t i = 0; i < info.nBuffers; i++) {
			bufferFrames[i].descSet = descSetArr[i];
		}

		std::vector<vk::DescriptorBufferInfo> descBufferInfos(info.nBuffers);
		std::vector<vk::WriteDescriptorSet> descBufferWrites(info.nBuffers);
		for (size_t i = 0; i < info.nBuffers; i++) {
			descBufferInfos[i] = vk::DescriptorBufferInfo()
				.setBuffer(bufferFrames[i].buffer)
				.setOffset(0)
				.setRange(sizeof(T));

			descBufferWrites[i] = vk::WriteDescriptorSet()
				.setDstSet(bufferFrames[i].descSet)
				.setDstBinding(info.binding)
				.setDstArrayElement(0)
				.setDescriptorType(vk::DescriptorType::eUniformBuffer)
				.setDescriptorCount(1)
				//
				.setPBufferInfo(&descBufferInfos[i])
				.setPImageInfo(nullptr)
				.setPTexelBufferView(nullptr);

		}
		info.deviceWrapper.logicalDevice.updateDescriptorSets(descBufferWrites, {});
	}

private:
	struct BufferFrame
	{
		void destroy(vma::Allocator& allocator)
		{
			allocator.destroyBuffer(buffer, alloc);
		}

		vk::Buffer buffer;
		vma::Allocation alloc;
		vma::AllocationInfo allocInfo;
		vk::DescriptorSet descSet;
	};
	RingBuffer<BufferFrame> bufferFrames;
};