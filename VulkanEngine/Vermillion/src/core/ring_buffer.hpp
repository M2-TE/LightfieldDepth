#pragma once

//tODO: remove this cyclic include
#include "wrappers/swapchain_wrapper.hpp"

struct RingFrame
{
	// command pools/buffers
	vk::CommandPool commandPools;
	vk::CommandBuffer commandBuffers;

	// sync objects
	vk::Semaphore imageAvailable;
	vk::Semaphore renderFinished;
	vk::Fence fence;

	// depth stencil
	std::pair<vk::Image, vma::Allocation> depthStencilAllocation;
	vk::ImageView depthStencilView;

	// swapchain images
	vk::Image swapchainImage;
	vk::ImageView swapchainImageView;
	vk::Framebuffer swapchainFramebuffer;
};

class RingBuffer
{
public:
	RingBuffer() = default;
	~RingBuffer() = default;
	ROF_COPY_MOVE_DELETE(RingBuffer)

public:
	void init(size_t nFrames) 
	{
		frames.resize(nFrames);
	}
	void create_all(DeviceWrapper& deviceWrapper, vma::Allocator& allocator, vk::RenderPass& renderPass, SwapchainWrapper& swapchainWrapper)
	{
		create_depth_stencils(deviceWrapper, allocator, swapchainWrapper);
		create_swapchain_images(deviceWrapper, swapchainWrapper);
		create_swapchain_image_views(deviceWrapper, swapchainWrapper);
		create_swapchain_framebuffers(deviceWrapper, swapchainWrapper, renderPass);
		create_sync_objects(deviceWrapper);
	}
	void destroy_all(DeviceWrapper& deviceWrapper, vma::Allocator& allocator)
	{
		destroy_depth_stencils(deviceWrapper, allocator);
		destroy_swapchain_image_views(deviceWrapper);
		destroy_swapchain_framebuffers(deviceWrapper);
		destroy_sync_objects(deviceWrapper);
	}

	// create ring frame resources
	void create_depth_stencils(DeviceWrapper& deviceWrapper, vma::Allocator& allocator, SwapchainWrapper& swapchainWrapper)
	{
		// Image
		vk::ImageCreateInfo imageCreateInfo;
		{
			imageCreateInfo = vk::ImageCreateInfo()
				.setPNext(nullptr)
				.setImageType(vk::ImageType::e2D)
				.setFormat(vk::Format::eD24UnormS8Uint)
				.setExtent(vk::Extent3D(swapchainWrapper.extent, 1)) // TODO: 1 or 0 in depth?
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

		// Image View
		{
			vk::ImageSubresourceRange subresourceRange = vk::ImageSubresourceRange()
				.setAspectMask(vk::ImageAspectFlagBits::eDepth | vk::ImageAspectFlagBits::eStencil)
				.setBaseMipLevel(0).setLevelCount(1)
				.setBaseArrayLayer(0).setLayerCount(1);


			for (size_t i = 0; i < frames.size(); i++) {
				vk::ImageViewCreateInfo imageViewInfo = vk::ImageViewCreateInfo()
					.setPNext(nullptr)
					.setViewType(vk::ImageViewType::e2D)
					.setImage(frames[i].depthStencilAllocation.first)
					.setFormat(imageCreateInfo.format)
					.setSubresourceRange(subresourceRange);
				frames[i].depthStencilView = deviceWrapper.logicalDevice.createImageView(imageViewInfo);
			}
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
	void create_swapchain_framebuffers(DeviceWrapper& deviceWrapper, SwapchainWrapper& swapchainWrapper, vk::RenderPass& renderPass)
	{
		size_t size = frames.size();

		for (size_t i = 0; i < size; i++) {
			std::array<vk::ImageView, 2> attachments = { frames[i].swapchainImageView, frames[i].depthStencilView };
			vk::FramebufferCreateInfo framebufferInfo = vk::FramebufferCreateInfo()
				.setRenderPass(renderPass)
				.setWidth(swapchainWrapper.extent.width)
				.setHeight(swapchainWrapper.extent.height)
				.setLayers(1)
				// attachments
				.setAttachmentCount(attachments.size()).setPAttachments(attachments.data());

			frames[i].swapchainFramebuffer = deviceWrapper.logicalDevice.createFramebuffer(framebufferInfo);
		}
	}
	void create_sync_objects(DeviceWrapper& deviceWrapper)
	{
		vk::Device& device = deviceWrapper.logicalDevice;

		vk::SemaphoreCreateInfo semaphoreInfo = vk::SemaphoreCreateInfo();
		vk::FenceCreateInfo fenceInfo = vk::FenceCreateInfo().setFlags(vk::FenceCreateFlagBits::eSignaled);
		for (size_t i = 0; i < frames.size(); i++) {
			RingFrame& frame = frames[i];
			frame.imageAvailable = device.createSemaphore(semaphoreInfo);
			frame.renderFinished = device.createSemaphore(semaphoreInfo);
			frame.fence = device.createFence(fenceInfo);
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
	void destroy_swapchain_framebuffers(DeviceWrapper& deviceWrapper)
	{
		for (size_t i = 0; i < frames.size(); i++) {
			deviceWrapper.logicalDevice.destroyFramebuffer(frames[i].swapchainFramebuffer);
		}
	}
	void destroy_sync_objects(DeviceWrapper& deviceWrapper)
	{
		vk::Device& device = deviceWrapper.logicalDevice;
		for (size_t i = 0; i < frames.size(); i++) {
			RingFrame& frame = frames[i];
			device.destroySemaphore(frame.imageAvailable);
			device.destroySemaphore(frame.renderFinished);
			device.destroyFence(frame.fence);
		}
	}

public:
	std::vector<RingFrame> frames;
	// TEMPTEMPTEMP:
	uint32_t currentFrame = 0;
};