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
	vk::Framebuffer swapchainFramebuffer;

	// gbuffer images
	std::pair<vk::Image, vma::Allocation> gPosAlloc;
	std::pair<vk::Image, vma::Allocation> gColAlloc;
	std::pair<vk::Image, vma::Allocation> gNormAlloc;
	vk::ImageView gPosImageView;
	vk::ImageView gColImageView;
	vk::ImageView gNormImageView;
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
		syncFrames.resize(nFrames);

		for (size_t i = 0; i < frames.size(); i++) {
			frames[i].index = i;
			syncFrames[i].index = i;
		}
	}
	void create_all(DeviceWrapper& deviceWrapper, vma::Allocator& allocator, vk::RenderPass& renderPass, SwapchainWrapper& swapchainWrapper)
	{
		create_depth_stencils(allocator, swapchainWrapper);
		create_depth_stencil_views(deviceWrapper);

		create_gbuffer_images(allocator, swapchainWrapper);
		create_gbuffer_image_views(deviceWrapper);

		create_swapchain_images(deviceWrapper, swapchainWrapper);
		create_swapchain_image_views(deviceWrapper, swapchainWrapper);
		create_swapchain_framebuffers(deviceWrapper, swapchainWrapper, renderPass);

		create_sync_objects(deviceWrapper);
		create_command_pools(deviceWrapper);
		create_command_buffers(deviceWrapper);
	}
	void destroy_all(DeviceWrapper& deviceWrapper, vma::Allocator& allocator)
	{
		destroy_depth_stencils(deviceWrapper, allocator);
		destroy_gbuffer(deviceWrapper, allocator);
		destroy_swapchain_image_views(deviceWrapper);
		destroy_swapchain_framebuffers(deviceWrapper);
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

	void create_gbuffer_images(vma::Allocator& allocator, SwapchainWrapper& swapchainWrapper)
	{
		vk::ImageCreateInfo imageCreateInfo = vk::ImageCreateInfo()
			.setPNext(nullptr)
			.setImageType(vk::ImageType::e2D)
			.setExtent(vk::Extent3D(swapchainWrapper.extent, 1))
			//
			.setMipLevels(1)
			.setArrayLayers(1)
			.setSamples(vk::SampleCountFlagBits::e1)
			.setTiling(vk::ImageTiling::eOptimal)
			.setUsage(vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eInputAttachment);

		vma::AllocationCreateInfo allocCreateInfo = vma::AllocationCreateInfo()
			.setUsage(vma::MemoryUsage::eAutoPreferDevice)
			.setFlags(vma::AllocationCreateFlagBits::eDedicatedMemory);


		// gPosBuffer
		imageCreateInfo.setFormat(vk::Format::eR16G16B16A16Sfloat); // 16-bit signed float
		imageCreateInfo.setFormat(vk::Format::eR32G32B32A32Sfloat); // 32-bit signed float
			//.setUsage()  // TODO: might need to set different usage flags
		for (size_t i = 0; i < frames.size(); i++) {
			frames[i].gPosAlloc = allocator.createImage(imageCreateInfo, allocCreateInfo, nullptr);
		}

		// gColBuffer
		imageCreateInfo.setFormat(vk::Format::eR8G8B8A8Srgb);
		for (size_t i = 0; i < frames.size(); i++) {
			frames[i].gColAlloc = allocator.createImage(imageCreateInfo, allocCreateInfo, nullptr);
		}

		// gNormBuffer
		imageCreateInfo.setFormat(vk::Format::eR8G8B8A8Snorm); // normalized between -1 and 1
			//.setUsage()  // TODO: might need to set different usage flags
		for (size_t i = 0; i < frames.size(); i++) {
			frames[i].gNormAlloc = allocator.createImage(imageCreateInfo, allocCreateInfo, nullptr);
		}
	}
	void create_gbuffer_image_views(DeviceWrapper& deviceWrapper)
	{
		vk::ImageSubresourceRange subresourceRange = vk::ImageSubresourceRange()
			.setAspectMask(vk::ImageAspectFlagBits::eColor)
			.setBaseMipLevel(0).setLevelCount(1)
			.setBaseArrayLayer(0).setLayerCount(1);

		vk::ImageViewCreateInfo imageViewInfo = vk::ImageViewCreateInfo()
			.setPNext(nullptr)
			.setViewType(vk::ImageViewType::e2D)
			.setFormat(vk::Format::eR8G8B8A8Srgb)
			.setSubresourceRange(subresourceRange);

		// gPosImageView
		imageViewInfo.setFormat(vk::Format::eR16G16B16A16Sfloat); // 16-bit signed float
		imageViewInfo.setFormat(vk::Format::eR32G32B32A32Sfloat); // 32-bit signed float
		for (size_t i = 0; i < frames.size(); i++) {
			imageViewInfo.setImage(frames[i].gPosAlloc.first);
			frames[i].gPosImageView = deviceWrapper.logicalDevice.createImageView(imageViewInfo);
		}

		// gColImageView
		imageViewInfo.setFormat(vk::Format::eR8G8B8A8Srgb);
		for (size_t i = 0; i < frames.size(); i++) {
			imageViewInfo.setImage(frames[i].gColAlloc.first);
			frames[i].gColImageView = deviceWrapper.logicalDevice.createImageView(imageViewInfo);
		}


		// gNormImageView
		imageViewInfo.setFormat(vk::Format::eR8G8B8A8Snorm); // normalized between -1 and 1
		for (size_t i = 0; i < frames.size(); i++) {
			imageViewInfo.setImage(frames[i].gNormAlloc.first);
			frames[i].gNormImageView = deviceWrapper.logicalDevice.createImageView(imageViewInfo);
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

		VMI_LOG("TODO: adjust attachment size of framebuffer");
		for (size_t i = 0; i < size; i++) {
			std::array<vk::ImageView, 5> attachments = { 
				frames[i].swapchainImageView, 
				frames[i].depthStencilView,
				frames[i].gPosImageView,
				frames[i].gColImageView,
				frames[i].gNormImageView
			};

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
	void destroy_gbuffer(DeviceWrapper& deviceWrapper, vma::Allocator& allocator)
	{
		for (size_t i = 0; i < frames.size(); i++) {
			allocator.destroyImage(frames[i].gColAlloc.first, frames[i].gColAlloc.second);
			allocator.destroyImage(frames[i].gPosAlloc.first, frames[i].gPosAlloc.second);
			allocator.destroyImage(frames[i].gNormAlloc.first, frames[i].gNormAlloc.second);
			deviceWrapper.logicalDevice.destroyImageView(frames[i].gColImageView);
			deviceWrapper.logicalDevice.destroyImageView(frames[i].gPosImageView);
			deviceWrapper.logicalDevice.destroyImageView(frames[i].gNormImageView);
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