#pragma once

class SwapchainWrapper
{
public:
	SwapchainWrapper() = default;
	~SwapchainWrapper() = default;
	ROF_COPY_MOVE_DELETE(SwapchainWrapper)

	void init(DeviceWrapper& deviceWrapper, Window& window)
	{
		choose_surface_format(deviceWrapper);
		choose_present_mode(deviceWrapper);
		choose_extent(deviceWrapper, window);

		create_swapchain(deviceWrapper, window);
		create_image_views(deviceWrapper);
		create_sync_objects(deviceWrapper);
	}
	void create_framebuffers(DeviceWrapper& deviceWrapper, vk::RenderPass& renderPass)
	{
		size_t size = images.size();
		framebuffers.resize(size);

		for (size_t i = 0; i < size; i++) {
			vk::FramebufferCreateInfo framebufferInfo = vk::FramebufferCreateInfo()
				.setRenderPass(renderPass)
				.setWidth(extent.width)
				.setHeight(extent.height)
				.setLayers(1)
				// attachments
				.setAttachmentCount(1)
				.setPAttachments(&imageViews[i]);

			framebuffers[i] = deviceWrapper.logicalDevice.createFramebuffer(framebufferInfo);
		}
	}
	void destroy(DeviceWrapper& deviceWrapper)
	{
		auto& device = deviceWrapper.logicalDevice;

		for (uint32_t i = 0; i < images.size(); i++) {
			device.destroyImageView(imageViews[i]);
			device.destroyFramebuffer(framebuffers[i]);
		}

		for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
			device.destroySemaphore(imageAvailableSemaphores[i]);
			device.destroySemaphore(renderFinishedSemaphores[i]);
			device.destroyFence(inFlightFences[i]);
		}
		device.destroySwapchainKHR(swapchain);
	}

	uint32_t acquire_image(DeviceWrapper& deviceWrapper)
	{
		// wait for fence of current frame before going any further
		vk::Result result = deviceWrapper.logicalDevice.waitForFences(inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);
		if (result != vk::Result::eSuccess) assert(false);

		// TODO: check which array index actually needs either imgResult.value (image index) or currentFrame
		vk::ResultValue imgResult = deviceWrapper.logicalDevice.acquireNextImageKHR(swapchain, UINT64_MAX, imageAvailableSemaphores[currentFrame], nullptr);
		switch (imgResult.result) {
			case vk::Result::eSuccess: break;
			case vk::Result::eSuboptimalKHR: VMI_LOG("Suboptimal image acquisition."); break;
			case vk::Result::eErrorOutOfDateKHR: VMI_ERR("Swapchain: KHR out of date."); assert(false);
			default: assert(false);
		}

		return imgResult.value;
	}
	void present(DeviceWrapper& deviceWrapper, vk::CommandBuffer& commandBuffer, uint32_t iImage)
	{
		vk::PipelineStageFlags waitStages = vk::PipelineStageFlagBits::eColorAttachmentOutput;
		vk::SubmitInfo submitInfo = vk::SubmitInfo()
			.setPWaitDstStageMask(&waitStages)
			// semaphores
			.setWaitSemaphoreCount(1)
			.setPWaitSemaphores(&imageAvailableSemaphores[currentFrame])
			.setSignalSemaphoreCount(1)
			.setPSignalSemaphores(&renderFinishedSemaphores[currentFrame])
			// command buffers
			.setCommandBufferCount(1)
			.setPCommandBuffers(&commandBuffer);
		deviceWrapper.logicalDevice.resetFences(inFlightFences[currentFrame]); // FENCE
		deviceWrapper.queue.submit(submitInfo, inFlightFences[currentFrame]);


		vk::PresentInfoKHR presentInfo = vk::PresentInfoKHR()
			.setPImageIndices(&iImage)
			// semaphores
			.setWaitSemaphoreCount(1)
			.setPWaitSemaphores(&renderFinishedSemaphores[currentFrame])
			// swapchains
			.setSwapchainCount(1)
			.setPSwapchains(&swapchain);
		vk::Result result = deviceWrapper.queue.presentKHR(&presentInfo);
		if (result != vk::Result::eSuccess) assert(false);

		// advance frame index
		currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
	}

private:
	void choose_surface_format(DeviceWrapper& deviceWrapper)
	{
		const auto& availableFormats = deviceWrapper.formats;
		for (const auto& availableFormat : availableFormats) {
			if (availableFormat.format == targetFormat && availableFormat.colorSpace == targetColorSpace) {
				surfaceFormat = availableFormat;
				return;
			}
		}

		// settle with first format if the requested one isnt available
		surfaceFormat = availableFormats[0];
	}
	void choose_present_mode(DeviceWrapper& deviceWrapper)
	{
		const auto& availablePresentModes = deviceWrapper.presentModes;
		for (const auto& availablePresentMode : availablePresentModes) {
			if (availablePresentMode == targetPresentMode) {
				presentMode = availablePresentMode;
				return;
			}
		}

		// settle with fifo present mode should the requested one not be available
		presentMode = vk::PresentModeKHR::eFifo;
	}
	void choose_extent(DeviceWrapper& deviceWrapper, Window& window)
	{
		auto& capabilities = deviceWrapper.capabilities;

		int width, height;
		SDL_Vulkan_GetDrawableSize(window.get_window(), &width, &height);

		extent = vk::Extent2D()
			.setWidth(width)
			.setHeight(height);
	}
	
	void create_swapchain(DeviceWrapper& deviceWrapper, Window& window)
	{
		// set number of images in the swapchain buffer
		// the driver needs minImageCount - 1 for itself, so we add one extra to ensure we have at least two
		uint32_t imageCount = deviceWrapper.capabilities.minImageCount + 1u;
		if (deviceWrapper.capabilities.maxImageCount > 0 && imageCount > deviceWrapper.capabilities.maxImageCount) {
			imageCount = deviceWrapper.capabilities.maxImageCount;
		}

		vk::SwapchainCreateInfoKHR swapchainInfo = vk::SwapchainCreateInfoKHR()
			// image settings
			.setMinImageCount(imageCount)
			.setImageFormat(surfaceFormat.format)
			.setImageColorSpace(surfaceFormat.colorSpace)
			.setImageExtent(extent)
			.setImageArrayLayers(1)
			.setImageUsage(vk::ImageUsageFlagBits::eColorAttachment) // or: eTransferDst for deferred rendering

			// when both queues are the same, create exclusive access, else create concurrent access
			.setImageSharingMode(vk::SharingMode::eExclusive)
			.setQueueFamilyIndexCount(0)
			.setPQueueFamilyIndices(nullptr)

			// both of these should be the same
			.setPreTransform(vk::SurfaceTransformFlagBitsKHR::eIdentity)
			.setPreTransform(deviceWrapper.capabilities.currentTransform)

			// misc
			.setSurface(window.get_vulkan_surface())
			.setCompositeAlpha(vk::CompositeAlphaFlagBitsKHR::eOpaque)
			.setPresentMode(presentMode)
			.setClipped(VK_TRUE)
			.setOldSwapchain(nullptr); // pointer to old swapchain on resize

		// finally, create swapchain
		swapchain = deviceWrapper.logicalDevice.createSwapchainKHR(swapchainInfo);

		// obtain swapchain images
		images = deviceWrapper.logicalDevice.getSwapchainImagesKHR(swapchain);
	}
	void create_image_views(DeviceWrapper& deviceWrapper)
	{
		vk::ComponentMapping mapping = vk::ComponentMapping()
			.setR(vk::ComponentSwizzle::eIdentity)
			.setG(vk::ComponentSwizzle::eIdentity)
			.setB(vk::ComponentSwizzle::eIdentity)
			.setA(vk::ComponentSwizzle::eIdentity);

		vk::ImageSubresourceRange subrange = vk::ImageSubresourceRange()
			.setAspectMask(vk::ImageAspectFlagBits::eColor)
			.setBaseMipLevel(0)
			.setLevelCount(1)
			.setBaseArrayLayer(0)
			.setLayerCount(1);

		imageViews.resize(images.size());
		for (size_t i = 0; i < images.size(); i++) {
			vk::ImageViewCreateInfo imageInfo = vk::ImageViewCreateInfo()
				.setImage(images[i])
				.setViewType(vk::ImageViewType::e2D)
				.setFormat(surfaceFormat.format)
				.setComponents(mapping)
				.setSubresourceRange(subrange);

			imageViews[i] = deviceWrapper.logicalDevice.createImageView(imageInfo);
		}
	}
	void create_sync_objects(DeviceWrapper& deviceWrapper)
	{
		vk::Device& device = deviceWrapper.logicalDevice;

		imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
		renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
		inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);
		vk::SemaphoreCreateInfo semaphoreInfo = vk::SemaphoreCreateInfo();
		vk::FenceCreateInfo fenceInfo = vk::FenceCreateInfo().setFlags(vk::FenceCreateFlagBits::eSignaled);
		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
			imageAvailableSemaphores[i] = device.createSemaphore(semaphoreInfo);
			renderFinishedSemaphores[i] = device.createSemaphore(semaphoreInfo);
			inFlightFences[i] = device.createFence(fenceInfo);
		}
	}
private:
	static constexpr vk::Format targetFormat = vk::Format::eR8G8B8A8Srgb;
	static constexpr vk::ColorSpaceKHR targetColorSpace = vk::ColorSpaceKHR::eSrgbNonlinear;
	static constexpr vk::PresentModeKHR targetPresentMode = vk::PresentModeKHR::eFifo; // vsync
	//static constexpr vk::PresentModeKHR targetPresentMode = vk::PresentModeKHR::eMailbox;


public:
	static constexpr int32_t MAX_FRAMES_IN_FLIGHT = 2;
	size_t currentFrame = 0;
	std::vector<vk::Semaphore> imageAvailableSemaphores;
	std::vector<vk::Semaphore> renderFinishedSemaphores;
	std::vector<vk::Fence> inFlightFences;

	vk::Extent2D extent; 
	vk::SurfaceFormatKHR surfaceFormat;
	vk::PresentModeKHR presentMode;

	vk::SwapchainKHR swapchain;
	std::vector<vk::Image> images;
	std::vector<vk::ImageView> imageViews;
	std::vector<vk::Framebuffer> framebuffers;
};