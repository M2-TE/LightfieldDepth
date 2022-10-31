#pragma once

#include "devices/device_wrapper.hpp"

class SyncFrameData
{
public:
	void init(DeviceWrapper& deviceWrapper)
	{
		create_semaphores(deviceWrapper);
		create_fence(deviceWrapper);
		create_command_pools(deviceWrapper);
		create_command_buffer(deviceWrapper);
	}
	void destroy(DeviceWrapper& deviceWrapper)
	{
		vk::Device& device = deviceWrapper.logicalDevice;
		device.destroySemaphore(imageAvailable);
		device.destroySemaphore(renderFinished);
		device.destroyFence(commandBufferFence);
		device.destroyCommandPool(commandPool);
	}

private:
	void create_semaphores(DeviceWrapper& deviceWrapper)
	{
		vk::SemaphoreCreateInfo semaphoreInfo = vk::SemaphoreCreateInfo();

		imageAvailable = deviceWrapper.logicalDevice.createSemaphore(semaphoreInfo);
		renderFinished = deviceWrapper.logicalDevice.createSemaphore(semaphoreInfo);
	}
	void create_fence(DeviceWrapper& deviceWrapper)
	{
		vk::FenceCreateInfo fenceInfo = vk::FenceCreateInfo()
			.setFlags(vk::FenceCreateFlagBits::eSignaled);

		commandBufferFence = deviceWrapper.logicalDevice.createFence(fenceInfo);
	}
	void create_command_pools(DeviceWrapper& deviceWrapper)
	{
		vk::CommandPoolCreateInfo commandPoolInfo;

		commandPoolInfo = vk::CommandPoolCreateInfo()
			.setQueueFamilyIndex(deviceWrapper.iQueue);

		commandPool = deviceWrapper.logicalDevice.createCommandPool(commandPoolInfo);

	}
	void create_command_buffer(DeviceWrapper& deviceWrapper)
	{
		vk::CommandBufferAllocateInfo commandBufferInfo = vk::CommandBufferAllocateInfo()
			.setCommandPool(commandPool)
			.setLevel(vk::CommandBufferLevel::ePrimary) // secondary are used by primary command buffers for e.g. common operations
			.setCommandBufferCount(1);
		commandBuffer = deviceWrapper.logicalDevice.allocateCommandBuffers(commandBufferInfo)[0];
	}

public:
	vk::Semaphore imageAvailable;
	vk::Semaphore renderFinished;
	vk::Fence commandBufferFence;

	// needs to be vector later (one for each thread)
	vk::CommandPool commandPool;
	vk::CommandBuffer commandBuffer;
};

template<class Data>
class RingBuffer
{
public:
	RingBuffer() = default;
	~RingBuffer() = default;
	ROF_COPY_MOVE_DELETE(RingBuffer)

public:
	template<typename... Args>
	void init(Args... args)
	{
		reset();
		for (size_t i = 0; i < frames.size(); i++) {
			frames[i].data.init(args...);
		}
	}
	template<typename... Args>
	void destroy(Args... args)
	{
		for (size_t i = 0; i < frames.size(); i++) {
			frames[i].data.destroy(args...);
		}
	}

	RingBuffer& set_size(uint32_t nFrames)
	{
		frames.resize(nFrames);
		reset();
		return *this;
	}
	uint32_t get_size() { return (uint32_t)frames.size(); }

	void reset()
	{
		// set up linked list
		for (size_t i = 0; i < frames.size() - 1; i++) {
			frames[i].pNext = &frames[i];
		}
		frames.back().pNext = &frames.front();
		pCurrent = &frames.front();
	}
	void advance()
	{
		pCurrent = pCurrent->pNext;
	}
	Data& get_current()
	{
		return pCurrent->data;
	}
	Data& get_next()
	{
		advance();
		return pCurrent->data;
	}
	Data& operator[](size_t i)
	{
		return frames[i].data;
	}
	std::vector<Data*> get_all()
	{
		std::vector<Data*> allData(frames.size);
		for (size_t i = 0; i < frames.size(); i++) {
			allData[i] = &frames[i].data;
		}
		return allData;
	}

private:
	struct RingFrame { RingFrame* pNext; Data data; };
	std::vector<RingFrame> frames;
	RingFrame* pCurrent;
};