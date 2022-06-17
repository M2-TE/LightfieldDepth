#pragma once

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
		// set up linked list
		for (size_t i = 0; i < frames.size() - 1; i++) {
			frames[i].pNext = &frames[i];
			frames[i].data.init(args...);
		}
		frames.back().pNext = &frames.front();

		// set initial active frame
		pCurrent = &frames.front();
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
		return *this;
	}
	uint32_t get_size() { return frames.size(); }

	Data& get_next()
	{
		pCurrent = pCurrent->pNext;
		return pCurrent->data;
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



// DEPRECATED

#include "wrappers/swapchain_wrapper.hpp"

struct RingFramee
{
	uint32_t index;

	// swapchain images
	vk::Image swapchainImage;
	vk::ImageView swapchainImageView;
};

class RingBufferr
{
public:
	RingBufferr() = default;
	~RingBufferr() = default;
	ROF_COPY_MOVE_DELETE(RingBufferr)

public:
	void init(DeviceWrapper& deviceWrapper, vma::Allocator& allocator, SwapchainWrapper& swapchainWrapper, size_t nFrames)
	{
		frames.resize(nFrames);

		for (size_t i = 0; i < frames.size(); i++) {
			frames[i].index = i;
		}

		create_swapchain_images(deviceWrapper, swapchainWrapper);
		create_swapchain_image_views(deviceWrapper, swapchainWrapper);
	}
	void destroy(DeviceWrapper& deviceWrapper, vma::Allocator& allocator)
	{
		destroy_swapchain_image_views(deviceWrapper);
	}

private:
	void create_swapchain_images(DeviceWrapper& deviceWrapper, SwapchainWrapper& swapchainWrapper)
	{
		std::vector<vk::Image> images = deviceWrapper.logicalDevice.getSwapchainImagesKHR(swapchainWrapper.swapchain);

		for (size_t i = 0; i < frames.size(); i++) {
			frames[i].swapchainImage = images[i];
		}
	}
	void create_swapchain_image_views(DeviceWrapper& deviceWrapper, SwapchainWrapper& swapchainWrapper)
	{
		vk::ComponentMapping mapping = vk::ComponentMapping()
			.setR(vk::ComponentSwizzle::eIdentity)
			.setG(vk::ComponentSwizzle::eIdentity)
			.setB(vk::ComponentSwizzle::eIdentity)
			.setA(vk::ComponentSwizzle::eIdentity);

		vk::ImageSubresourceRange subrange = vk::ImageSubresourceRange()
			.setAspectMask(vk::ImageAspectFlagBits::eColor)
			.setLevelCount(1).setBaseMipLevel(0)
			.setLayerCount(1).setBaseArrayLayer(0);

		for (size_t i = 0; i < frames.size(); i++) {
			vk::ImageViewCreateInfo imageInfo = vk::ImageViewCreateInfo()
				.setImage(frames[i].swapchainImage)
				.setViewType(vk::ImageViewType::e2D)
				.setFormat(swapchainWrapper.surfaceFormat.format)
				.setComponents(mapping)
				.setSubresourceRange(subrange);

			frames[i].swapchainImageView = deviceWrapper.logicalDevice.createImageView(imageInfo);
		}
	}

	// destroy ring frame resources
	void destroy_swapchain_image_views(DeviceWrapper& deviceWrapper)
	{
		for (size_t i = 0; i < frames.size(); i++) {
			deviceWrapper.logicalDevice.destroyImageView(frames[i].swapchainImageView);
		}
	}

public:
	std::vector<RingFramee> frames;
};