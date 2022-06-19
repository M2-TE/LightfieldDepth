#pragma once

template<class T>
class UniformBufferWrapper
{
public:
	UniformBufferWrapper() = default;
	~UniformBufferWrapper() = default;
	ROF_COPY_MOVE_DELETE(UniformBufferWrapper)

public:
	void init(DeviceWrapper& deviceWrapper, vma::Allocator& allocator, vk::DescriptorPool& descPool, uint32_t binding, vk::ShaderStageFlags stageFlags, size_t nBuffers)
	{
		create_buffer(allocator, nBuffers);
		create_descriptor_set_layout(deviceWrapper, binding, stageFlags);
		create_descriptor_sets(deviceWrapper, descPool, binding, nBuffers);
	}
	void destroy(DeviceWrapper& deviceWrapper, vma::Allocator& allocator)
	{
		for (size_t i = 0; i < buffers.size(); i++) {
			allocator.destroyBuffer(buffers[i].first, buffers[i].second);
		}
		deviceWrapper.logicalDevice.destroyDescriptorSetLayout(descSetLayout);
	}
	void update(size_t iBuffer)
	{
		// already mapped, so just copy over
		memcpy(allocInfos[iBuffer].pMappedData, &data, sizeof(T));
	}

	vk::DescriptorSet get_desc_set(uint32_t iDescSet)
	{
		return descSets[iDescSet];
	}
	vk::DescriptorSetLayout& get_desc_set_layout()
	{
		return descSetLayout;
	}

private:
	void create_buffer(vma::Allocator& allocator, size_t nBuffers)
	{
		// buffer
		vk::BufferCreateInfo bufferInfo = vk::BufferCreateInfo()
			.setSize(sizeof(T))
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
	void create_descriptor_set_layout(DeviceWrapper& deviceWrapper, uint32_t binding, vk::ShaderStageFlags stageFlags)
	{
		vk::DescriptorSetLayoutBinding layoutBinding = vk::DescriptorSetLayoutBinding()
			.setBinding(binding)
			.setStageFlags(stageFlags)
			.setDescriptorType(vk::DescriptorType::eUniformBuffer)
			.setDescriptorCount(1)
			.setPImmutableSamplers(nullptr);

		// create descriptor set layout from all the bindings
		vk::DescriptorSetLayoutCreateInfo createInfo = vk::DescriptorSetLayoutCreateInfo()
			.setBindingCount(1)
			.setBindings(layoutBinding);
		descSetLayout = deviceWrapper.logicalDevice.createDescriptorSetLayout(createInfo);
	}
	void create_descriptor_sets(DeviceWrapper& deviceWrapper, vk::DescriptorPool& descPool, uint32_t binding, uint32_t nBuffers)
	{
		// duplicate layout to have one for each frame in flight
		std::vector<vk::DescriptorSetLayout> layouts(nBuffers, descSetLayout);

		// allocate the descriptor sets using descriptor pool
		vk::DescriptorSetAllocateInfo allocInfo = vk::DescriptorSetAllocateInfo()
			.setDescriptorPool(descPool)
			.setDescriptorSetCount(layouts.size())
			.setPSetLayouts(layouts.data());
		descSets = deviceWrapper.logicalDevice.allocateDescriptorSets(allocInfo);

		std::vector<vk::DescriptorBufferInfo> descBufferInfos(nBuffers);
		std::vector<vk::WriteDescriptorSet> descBufferWrites(nBuffers);
		for (size_t i = 0; i < nBuffers; i++) {
			descBufferInfos[i] = vk::DescriptorBufferInfo()
				.setBuffer(buffers[i].first)
				.setOffset(0)
				.setRange(sizeof(T));

			descBufferWrites[i] = vk::WriteDescriptorSet()
				.setDstSet(descSets[i])
				.setDstBinding(binding)
				.setDstArrayElement(0)
				.setDescriptorType(vk::DescriptorType::eUniformBuffer)
				.setDescriptorCount(1)
				//
				.setPBufferInfo(&descBufferInfos[i])
				.setPImageInfo(nullptr)
				.setPTexelBufferView(nullptr);

		}
		deviceWrapper.logicalDevice.updateDescriptorSets(descBufferWrites, {});
	}

public:
	T data;

private:
	// TODO: use Dynamic Uniform Buffers
	std::vector<std::pair<vk::Buffer, vma::Allocation>> buffers;
	std::vector<vma::AllocationInfo> allocInfos;

	vk::DescriptorSetLayout descSetLayout; 
	std::vector<vk::DescriptorSet> descSets;
};