#pragma once

class Renderer
{
public:
	Renderer() = default;
	~Renderer() = default;
	ROF_COPY_MOVE_DELETE(Renderer)

public:
	void init(DeviceManager& deviceManager, Window& window)
	{
		create_swapchain(deviceManager, window);
		create_swapchain_image_views(deviceManager);

		create_render_pass(deviceManager);

		//CreateRenderPass();
		//CreateGraphicsPipeline();
		//CreateFramebuffers();
		//CreateCommandPool();
		//CreateCommandBuffers();
		//CreateSyncObjects();
	}

	void destroy()
	{
		// TODO
	}

private:
	// swapchain and its related stuff
	vk::SurfaceFormatKHR choose_surface_format(const std::vector<vk::SurfaceFormatKHR>& availableFormats)
	{
		for (const auto& availableFormat : availableFormats) {
			if (availableFormat.format == vk::Format::eR8G8B8A8Srgb && availableFormat.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear) {
				return availableFormat;
			}
		}

		// settle with first format if the requested one isnt available
		return availableFormats[0];
	}
	vk::PresentModeKHR choose_present_mode(const std::vector<vk::PresentModeKHR>& availablePresentModes)
	{
		for (const auto& availablePresentMode : availablePresentModes) {
			if (availablePresentMode == vk::PresentModeKHR::eMailbox) {
				return availablePresentMode;
			}
		}

		// settle with fifo present mode should the requested one not be available
		return vk::PresentModeKHR::eFifo;
	}
	vk::Extent2D choose_extent(const vk::SurfaceCapabilitiesKHR& capabilities, SDL_Window* pWindow)
	{
		if (capabilities.currentExtent.width != UINT32_MAX) {
			return capabilities.currentExtent;
		}
		else {
			int width, height;
			SDL_GL_GetDrawableSize(pWindow, &width, &height);

			vk::Extent2D actualExtent = vk::Extent2D()
				.setWidth(std::clamp(static_cast<uint32_t>(width), capabilities.minImageExtent.width, capabilities.maxImageExtent.width))
				.setHeight(std::clamp(static_cast<uint32_t>(height), capabilities.minImageExtent.height, capabilities.maxImageExtent.height));

			return actualExtent;
		}
	}
	void create_swapchain(DeviceManager& deviceManager, Window& window)
	{
		auto& deviceWrapper = deviceManager.get_device_wrapper();
		vk::Device& device = deviceManager.get_logical_device();

		vk::SurfaceFormatKHR surfaceFormat = choose_surface_format(deviceWrapper.swapchain.formats);
		vk::PresentModeKHR presentMode = choose_present_mode(deviceWrapper.swapchain.presentModes);
		swapchainExtent = choose_extent(deviceWrapper.swapchain.capabilities, window.get_window()); // in theory dont need a func call for this (member var)
		swapchainImageFormat = surfaceFormat.format;

		// set number of images in the swapchain buffer
		uint32_t imageCount = deviceWrapper.swapchain.capabilities.minImageCount + 1u;
		if (deviceWrapper.swapchain.capabilities.maxImageCount > 0 && imageCount > deviceWrapper.swapchain.capabilities.maxImageCount) {
			imageCount = deviceWrapper.swapchain.capabilities.maxImageCount;
		}

		// figure out if present and graphics queues are the same
		uint32_t queueFamilyIndices[] = { deviceWrapper.indices.iGraphicsFamily.value(), deviceWrapper.indices.iPresentFamily.value() };
		bool bExclusiveAccess = deviceWrapper.indices.iGraphicsFamily == deviceWrapper.indices.iPresentFamily;

		vk::SwapchainCreateInfoKHR swapchainInfo = vk::SwapchainCreateInfoKHR()
			// image settings
			.setMinImageCount(imageCount)
			.setImageFormat(surfaceFormat.format)
			.setImageColorSpace(surfaceFormat.colorSpace)
			.setImageExtent(swapchainExtent)
			.setImageArrayLayers(1)
			.setImageUsage(vk::ImageUsageFlagBits::eColorAttachment) // or: eTransferDst for deferred rendering

			// when both queues are the same, create exclusive access, else create concurrent access
			.setImageSharingMode(bExclusiveAccess ? vk::SharingMode::eExclusive : vk::SharingMode::eConcurrent)
			.setQueueFamilyIndexCount(bExclusiveAccess ? 0u : 2u) // 2 queues
			.setPQueueFamilyIndices(bExclusiveAccess ? nullptr : queueFamilyIndices)

			// both of these should be the same
			.setPreTransform(vk::SurfaceTransformFlagBitsKHR::eIdentity)
			.setPreTransform(deviceWrapper.swapchain.capabilities.currentTransform)

			// misc
			.setSurface(window.get_vulkan_surface())
			.setCompositeAlpha(vk::CompositeAlphaFlagBitsKHR::eOpaque)
			.setPresentMode(presentMode)
			.setClipped(VK_TRUE)
			.setOldSwapchain(nullptr); // pointer to old swapchain on resize

		// finally, create swapchain
		swapchain = device.createSwapchainKHR(swapchainInfo);

		// obtain swapchain images
		swapchainImages = device.getSwapchainImagesKHR(swapchain);
	}
	
	void create_swapchain_image_views(DeviceManager& deviceManager)
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

		auto& device = deviceManager.get_logical_device();
		swapchainImageViews.resize(swapchainImages.size());
		for (size_t i = 0; i < swapchainImages.size(); i++) {
			vk::ImageViewCreateInfo imageInfo = vk::ImageViewCreateInfo()
				.setImage(swapchainImages[i])
				.setViewType(vk::ImageViewType::e2D)
				.setFormat(swapchainImageFormat)
				.setComponents(mapping)
				.setSubresourceRange(subrange);

			swapchainImageViews[i] = device.createImageView(imageInfo);
		}
	}

	void create_render_pass(DeviceManager& deviceManager)
	{
		vk::AttachmentDescription colorAttachment = vk::AttachmentDescription()
			.setFormat(swapchainImageFormat)
			.setSamples(vk::SampleCountFlagBits::e1)
			.setLoadOp(vk::AttachmentLoadOp::eClear)
			.setStoreOp(vk::AttachmentStoreOp::eStore)
			.setStencilLoadOp(vk::AttachmentLoadOp::eDontCare)
			.setStencilStoreOp(vk::AttachmentStoreOp::eDontCare)
			.setInitialLayout(vk::ImageLayout::eUndefined)
			.setFinalLayout(vk::ImageLayout::ePresentSrcKHR);

		vk::AttachmentReference colorAttachmentRef = vk::AttachmentReference()
			.setAttachment(0)
			.setLayout(vk::ImageLayout::eColorAttachmentOptimal);

		vk::SubpassDescription subpassDesc = vk::SubpassDescription()
			.setPipelineBindPoint(vk::PipelineBindPoint::eGraphics)
			.setColorAttachmentCount(1)
			.setPColorAttachments(&colorAttachmentRef)
			.setPInputAttachments(nullptr)
			.setPResolveAttachments(nullptr)
			.setPDepthStencilAttachment(nullptr)
			.setPPreserveAttachments(nullptr);

		vk::SubpassDependency dependency = vk::SubpassDependency()
			// src
			.setSrcSubpass(VK_SUBPASS_EXTERNAL)
			.setSrcStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput)
			.setSrcAccessMask(vk::AccessFlagBits::eNoneKHR)
			// dst
			.setDstSubpass(0u)
			.setDstStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput)
			.setDstAccessMask(vk::AccessFlagBits::eColorAttachmentWrite);

		vk::RenderPassCreateInfo renderPassInfo = vk::RenderPassCreateInfo()
			.setAttachmentCount(1)
			.setPAttachments(&colorAttachment)
			.setSubpassCount(1)
			.setPSubpasses(&subpassDesc)
			.setDependencyCount(1)
			.setPDependencies(&dependency);

		renderPass = deviceManager.get_logical_device().createRenderPass(renderPassInfo);
	}

	vk::ShaderModule create_shader_module(DeviceManager& deviceManager, const unsigned char* data, size_t size)
	{
		vk::ShaderModuleCreateInfo shaderInfo = vk::ShaderModuleCreateInfo()
			.setCodeSize(size)
			.setPCode(reinterpret_cast<const uint32_t*>(data)); // data alignment?
		return deviceManager.get_logical_device().createShaderModule(shaderInfo);
	}
	void create_graphics_pipeline()
	{
		// TODO
	}

private:
	// lazy constants (settings?)
	const int MAX_FRAMES_IN_FLIGHT = 2;

	vk::Queue qGraphics, qPresent;

	vk::SwapchainKHR swapchain;
	vk::Format swapchainImageFormat;
	std::vector<vk::Image> swapchainImages;
	std::vector<vk::ImageView> swapchainImageViews;
	std::vector<vk::Framebuffer> swapchainFramebuffers;

	vk::Extent2D swapchainExtent;
	vk::ShaderModule vertShaderModule;
	vk::ShaderModule fragShaderModule;
	vk::RenderPass renderPass;
	vk::PipelineLayout pipelineLayout;
	vk::Pipeline graphicsPipeline;
	vk::CommandPool commandPool;
	std::vector<vk::CommandBuffer> commandBuffers;

	std::vector<vk::Semaphore> imageAvailableSemaphores;
	std::vector<vk::Semaphore> renderFinishedSemaphores;
	std::vector<vk::Fence> inFlightFences;
	std::vector<vk::Fence> imagesInFlight;
	size_t currentFrame = 0;
};