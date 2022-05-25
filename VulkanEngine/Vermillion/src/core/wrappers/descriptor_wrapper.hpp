#pragma once

#include "uniform_buffer_wrapper.hpp"

class DescriptorWrapper
{
public:
	DescriptorWrapper() = default;
	~DescriptorWrapper() = default;
	ROF_COPY_MOVE_DELETE(DescriptorWrapper)

public:
	void init(DeviceWrapper& deviceWrapper)
	{
		create_desc_pool(deviceWrapper);
	}
	void destroy(DeviceWrapper& deviceWrapper)
	{
		deviceWrapper.logicalDevice.destroyDescriptorPool(descPool);
		deviceWrapper.logicalDevice.destroyDescriptorSetLayout(descSetLayout);
	}
	void update(DeviceWrapper& deviceWrapper, uint32_t nFramesInFlight)
	{
		// create descriptor set layout from all the bindings
		vk::DescriptorSetLayoutCreateInfo createInfo = vk::DescriptorSetLayoutCreateInfo()
			.setBindingCount(layoutBindings.size())
			.setPBindings(layoutBindings.data());
		descSetLayout = deviceWrapper.logicalDevice.createDescriptorSetLayout(createInfo);

		// duplicate layout to have one for each frame in flight
		std::vector<vk::DescriptorSetLayout> layouts(nFramesInFlight, descSetLayout);

		// allocate the descriptor sets using descriptor pool
		vk::DescriptorSetAllocateInfo allocInfo = vk::DescriptorSetAllocateInfo()
			.setDescriptorPool(descPool)
			.setDescriptorSetCount(layouts.size())
			.setPSetLayouts(layouts.data());
		descSets = deviceWrapper.logicalDevice.allocateDescriptorSets(allocInfo);


		size_t n = layoutBindings.size() * nFramesInFlight;
		std::vector<vk::DescriptorBufferInfo> descBufferInfos(n, vk::DescriptorBufferInfo());
		std::vector<vk::WriteDescriptorSet> descBufferWrites(n, vk::WriteDescriptorSet());
		for (size_t iFrame = 0; iFrame < nFramesInFlight; iFrame++) {
			for (size_t iBinding = 0; iBinding < layoutBindings.size(); iBinding++) {

				// index into the vectors, they are basically n dimension arrays
				size_t i = iFrame * layoutBindings.size() + iBinding;

				descBufferInfos[i]
					.setBuffer(bufferInfos[iBinding].buffers[iFrame])
					.setRange(bufferInfos[iBinding].size)
					.setOffset(0);

				descBufferWrites[i]
					.setDstSet(descSets[iFrame])
					.setDstBinding(layoutBindings[iBinding].binding)
					.setDstArrayElement(0)
					.setDescriptorType(layoutBindings[iBinding].descriptorType)
					.setDescriptorCount(1)
					// based on buffer type:
					.setPBufferInfo(&descBufferInfos[i])
					.setPImageInfo(nullptr)
					.setPTexelBufferView(nullptr);
			}
		}
		deviceWrapper.logicalDevice.updateDescriptorSets(descBufferWrites, {});
	}

	template<class T>
	void add_uniform_buffer(UniformBufferWrapper<T>& uniformBuffer, uint32_t binding, vk::ShaderStageFlags stages)
	{
		auto descSetLayout = vk::DescriptorSetLayoutBinding()
			.setBinding(binding)
			.setStageFlags(stages)
			.setDescriptorType(uniformBuffer.get_descriptor_type())
			.setDescriptorCount(1)
			.setPImmutableSamplers(nullptr);

		uniformBuffer.iLayoutBinding = layoutBindings.size();
		layoutBindings.push_back(descSetLayout);

		// save pointers to the buffers and the size of the buffers themselves
		BufferInfo infos = {};
		infos.size = sizeof(T);
		infos.buffers.reserve(uniformBuffer.buffers.size());
		for (size_t i = 0; i < uniformBuffer.buffers.size(); i++) {
			infos.buffers.push_back(uniformBuffer.buffers[i].first);
		}
		bufferInfos.push_back(infos);
	}

private:
	void create_desc_pool(DeviceWrapper& deviceWrapper)
	{
		std::array<vk::DescriptorPoolSize, 1>  poolSizes =
		{
			vk::DescriptorPoolSize(vk::DescriptorType::eUniformBuffer, poolSize)
			// TODO: other stuff this pool will need
		};

		vk::DescriptorPoolCreateInfo info = vk::DescriptorPoolCreateInfo()
			.setFlags(vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet)
			.setMaxSets(poolSize * (uint32_t)poolSizes.size())
			.setPoolSizeCount((uint32_t)poolSizes.size())
			.setPPoolSizes(poolSizes.data());
		descPool = deviceWrapper.logicalDevice.createDescriptorPool(info);
	}

private:
	static constexpr uint32_t poolSize = 1000;

	vk::DescriptorPool descPool;
	vk::DescriptorSetLayout descSetLayout;
	std::vector<vk::DescriptorSet> descSets; // TODO

	struct BufferInfo { std::vector<vk::Buffer> buffers; uint32_t size; };
	std::vector<BufferInfo> bufferInfos; // pointers to buffers + their size
	std::vector<vk::DescriptorSetLayoutBinding> layoutBindings; // contains the bindings of all active uniform buffers
};