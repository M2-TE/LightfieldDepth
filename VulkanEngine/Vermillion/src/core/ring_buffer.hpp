#pragma once

#include "wrappers/swapchain_wrapper.hpp"

struct SyncFrame
{
	uint32_t index;

	// sync objects
	vk::Semaphore imageAvailable;
	vk::Semaphore renderFinished;
	vk::Fence fence;
};
struct RingFrame
{
	uint32_t index;

	// command pools/buffers
	vk::CommandPool commandPool;
	vk::CommandBuffer commandBuffer;

	// depth stencil
	std::pair<vk::Image, vma::Allocation> depthStencilAllocation;
	vk::ImageView depthStencilView;

	// swapchain images
	vk::Image swapchainImage;
	vk::ImageView swapchainImageView;
};

class RingBuffer
{
public:
	RingBuffer() = default;
	~RingBuffer() = default;
	ROF_COPY_MOVE_DELETE(RingBuffer)

public:
	void init(DeviceWrapper& deviceWrapper, vma::Allocator& allocator, SwapchainWrapper& swapchainWrapper, size_t nFrames)
	{
		frames.resize(nFrames);
		syncFrames.resize(nFrames);

		for (size_t i = 0; i < frames.size(); i++) {
			frames[i].index = i;
			syncFrames[i].index = i;
		}

		create_depth_stencils(allocator, swapchainWrapper);
		create_depth_stencil_views(deviceWrapper);

		create_swapchain_images(deviceWrapper, swapchainWrapper);
		create_swapchain_image_views(deviceWrapper, swapchainWrapper);

		create_sync_objects(deviceWrapper);
		create_command_pools(deviceWrapper);
		create_command_buffers(deviceWrapper);
	}
	void destroy(DeviceWrapper& deviceWrapper, vma::Allocator& allocator)
	{
		destroy_depth_stencils(deviceWrapper, allocator);
		destroy_swapchain_image_views(deviceWrapper);
		destroy_sync_objects(deviceWrapper);
		destroy_command_pools(deviceWrapper);
	}
	RingBuffer& advance()
	{
		iSyncFrame = (iSyncFrame + 1) % frames.size();
		return *this;
	}
	SyncFrame& get_sync_frame()
	{
		return syncFrames[iSyncFrame];
	}

private:
	// create ring frame resources
	void create_depth_stencils(vma::Allocator& allocator, SwapchainWrapper& swapchainWrapper)
	{
		vk::ImageCreateInfo imageCreateInfo = vk::ImageCreateInfo()
			.setPNext(nullptr)
			.setImageType(vk::ImageType::e2D)
			.setFormat(vk::Format::eD24UnormS8Uint)
			.setExtent(vk::Extent3D(swapchainWrapper.extent, 1))
			//
			.setMipLevels(1)
			.setArrayLayers(1)
			.setSamples(vk::SampleCountFlagBits::e1)
			.setTiling(vk::ImageTiling::eOptimal)
			.setUsage(vk::ImageUsageFlagBits::eDepthStencilAttachment);

		vma::AllocationCreateInfo allocCreateInfo = vma::AllocationCreateInfo()
			.setUsage(vma::MemoryUsage::eAutoPreferDevice)
			.setFlags(vma::AllocationCreateFlagBits::eDedicatedMemory);

		for (size_t i = 0; i < frames.size(); i++) {
			frames[i].depthStencilAllocation = allocator.createImage(imageCreateInfo, allocCreateInfo, nullptr);
		}
	}
	void create_depth_stencil_views(DeviceWrapper& deviceWrapper)
	{
		vk::ImageSubresourceRange subresourceRange = vk::ImageSubresourceRange()
			.setAspectMask(vk::ImageAspectFlagBits::eDepth | vk::ImageAspectFlagBits::eStencil)
			.setBaseMipLevel(0).setLevelCount(1)
			.setBaseArrayLayer(0).setLayerCount(1);

		vk::ImageViewCreateInfo imageViewInfo = vk::ImageViewCreateInfo()
			.setPNext(nullptr)
			.setViewType(vk::ImageViewType::e2D)
			.setFormat(vk::Format::eD24UnormS8Uint)
			.setSubresourceRange(subresourceRange);

		for (size_t i = 0; i < frames.size(); i++) {
			imageViewInfo.setImage(frames[i].depthStencilAllocation.first);
			frames[i].depthStencilView = deviceWrapper.logicalDevice.createImageView(imageViewInfo);
		}
	}

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
	
	void create_sync_objects(DeviceWrapper& deviceWrapper)
	{
		vk::Device& device = deviceWrapper.logicalDevice;

		vk::SemaphoreCreateInfo semaphoreInfo = vk::SemaphoreCreateInfo();
		vk::FenceCreateInfo fenceInfo = vk::FenceCreateInfo().setFlags(vk::FenceCreateFlagBits::eSignaled);
		for (size_t i = 0; i < frames.size(); i++) {
			SyncFrame& frame = syncFrames[i];
			frame.imageAvailable = device.createSemaphore(semaphoreInfo);
			frame.renderFinished = device.createSemaphore(semaphoreInfo);
			frame.fence = device.createFence(fenceInfo);
		}
	}
	void create_command_pools(DeviceWrapper& deviceWrapper)
	{
		vk::CommandPoolCreateInfo commandPoolInfo;

		commandPoolInfo = vk::CommandPoolCreateInfo()
			.setQueueFamilyIndex(deviceWrapper.iQueue);
		for (uint32_t i = 0; i < frames.size(); i++) {
			frames[i].commandPool = deviceWrapper.logicalDevice.createCommandPool(commandPoolInfo);
		}

	}
	void create_command_buffers(DeviceWrapper& deviceWrapper)
	{
		for (uint32_t i = 0; i < frames.size(); i++) {
			vk::CommandBufferAllocateInfo commandBufferInfo = vk::CommandBufferAllocateInfo()
				.setCommandPool(frames[i].commandPool)
				.setLevel(vk::CommandBufferLevel::ePrimary) // secondary are used by primary command buffers for e.g. common operations
				.setCommandBufferCount(1);
			frames[i].commandBuffer = deviceWrapper.logicalDevice.allocateCommandBuffers(commandBufferInfo)[0];
		}
	}

	// destroy ring frame resources
	void destroy_depth_stencils(DeviceWrapper& deviceWrapper, vma::Allocator& allocator)
	{
		for (size_t i = 0; i < frames.size(); i++) {
			allocator.destroyImage(frames[i].depthStencilAllocation.first, frames[i].depthStencilAllocation.second);
			deviceWrapper.logicalDevice.destroyImageView(frames[i].depthStencilView);
		}
	}

	void destroy_swapchain_image_views(DeviceWrapper& deviceWrapper)
	{
		for (size_t i = 0; i < frames.size(); i++) {
			deviceWrapper.logicalDevice.destroyImageView(frames[i].swapchainImageView);
		}
	}

	void destroy_sync_objects(DeviceWrapper& deviceWrapper)
	{
		vk::Device& device = deviceWrapper.logicalDevice;
		for (size_t i = 0; i < frames.size(); i++) {
			SyncFrame& frame = syncFrames[i];
			device.destroySemaphore(frame.imageAvailable);
			device.destroySemaphore(frame.renderFinished);
			device.destroyFence(frame.fence);
		}
	}
	void destroy_command_pools(DeviceWrapper& deviceWrapper)
	{
		for (size_t i = 0; i < frames.size(); i++) {
			deviceWrapper.logicalDevice.destroyCommandPool(frames[i].commandPool);
		}
	}

public:
	std::vector<RingFrame> frames;
private:
	std::vector<SyncFrame> syncFrames;
	size_t iSyncFrame = 0;
};