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
	}
	void create_framebuffers(DeviceWrapper& deviceWrapper, vk::RenderPass& renderPass)
	{
		framebuffers.resize(imageViews.size());

		for (size_t i = 0; i < imageViews.size(); i++) {
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
		device.destroySwapchainKHR(swapchain);
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

private:
	static constexpr vk::Format targetFormat = vk::Format::eR8G8B8A8Srgb;
	static constexpr vk::ColorSpaceKHR targetColorSpace = vk::ColorSpaceKHR::eSrgbNonlinear;
	static constexpr vk::PresentModeKHR targetPresentMode = vk::PresentModeKHR::eFifo; // vsync
	//static constexpr vk::PresentModeKHR targetPresentMode = vk::PresentModeKHR::eMailbox;

	//static constexpr int32_t MAX_FRAMES_IN_FLIGHT = 2;
	//size_t currentFrame = 0;

public:
	vk::Extent2D extent; 
	vk::SurfaceFormatKHR surfaceFormat;
	vk::PresentModeKHR presentMode;

	vk::SwapchainKHR swapchain;
	std::vector<vk::Image> images;
	std::vector<vk::ImageView> imageViews;
	std::vector<vk::Framebuffer> framebuffers;
};