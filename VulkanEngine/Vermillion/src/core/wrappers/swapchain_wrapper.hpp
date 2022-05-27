#pragma once

class SwapchainWrapper
{
public:
	SwapchainWrapper() = default;
	~SwapchainWrapper() = default;
	ROF_COPY_MOVE_DELETE(SwapchainWrapper)

	void init(DeviceWrapper& deviceWrapper, Window& window, uint32_t nImages)
	{
		choose_surface_format(deviceWrapper);
		choose_present_mode(deviceWrapper);
		choose_extent(deviceWrapper, window);

		create_swapchain(deviceWrapper, window, nImages);
	}
	void destroy(DeviceWrapper& deviceWrapper)
	{
		deviceWrapper.logicalDevice.destroySwapchainKHR(swapchain);
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
	void create_swapchain(DeviceWrapper& deviceWrapper, Window& window, uint32_t nImages)
	{
		if (deviceWrapper.capabilities.minImageCount > nImages) VMI_ERR("Swapchain has higher minimum image count requirement");

		vk::SwapchainCreateInfoKHR swapchainInfo = vk::SwapchainCreateInfoKHR()
			// image settings
			.setMinImageCount(nImages)
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
	}

private:
	static constexpr vk::Format targetFormat = vk::Format::eR8G8B8A8Srgb;
	static constexpr vk::ColorSpaceKHR targetColorSpace = vk::ColorSpaceKHR::eSrgbNonlinear;
	static constexpr vk::PresentModeKHR targetPresentMode = vk::PresentModeKHR::eFifo; // vsync

public:
	vk::SwapchainKHR swapchain;
	vk::SurfaceFormatKHR surfaceFormat;
	vk::PresentModeKHR presentMode;
	vk::Extent2D extent;
};