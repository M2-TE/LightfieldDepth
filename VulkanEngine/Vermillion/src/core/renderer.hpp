#pragma once

#include "shaders.hpp"

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
		create_graphics_pipeline(deviceManager);

		create_framebuffers(deviceManager);
		create_command_pool(deviceManager);
		create_command_buffers(deviceManager);

		create_sync_objects(deviceManager);
	}
	void destroy(DeviceManager& deviceManager)
	{
		vk::Device& device = deviceManager.get_logical_device();

		for (vk::ImageView imageView : swapchainImageViews) device.destroyImageView(imageView);
		for (vk::Framebuffer framebuffer : swapchainFramebuffers) device.destroyFramebuffer(framebuffer);
		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
			device.destroySemaphore(imageAvailableSemaphores[i]);
			device.destroySemaphore(renderFinishedSemaphores[i]);
			device.destroyFence(inFlightFences[i]);
		}

		device.destroyShaderModule(vs);
		device.destroyShaderModule(ps);
		device.destroyPipeline(graphicsPipeline);
		device.destroyPipelineLayout(pipelineLayout);
		device.destroyRenderPass(renderPass);
		device.destroySwapchainKHR(swapchain);
		device.destroyCommandPool(commandPool);
	}

	void render()
	{

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
	vk::ShaderModule create_shader_module(vk::Device& device, const unsigned char* data, size_t size)
	{
		vk::ShaderModuleCreateInfo shaderInfo = vk::ShaderModuleCreateInfo()
			.setCodeSize(size)
			.setPCode(reinterpret_cast<const uint32_t*>(data)); // data alignment?
		return device.createShaderModule(shaderInfo);
	}
	void create_graphics_pipeline(DeviceManager& deviceManager)
	{
		vk::Device& device = deviceManager.get_logical_device();
		vs = create_shader_module(device, shader_vs, shader_vs_size);
		ps = create_shader_module(device, shader_ps, shader_ps_size);

		vk::PipelineShaderStageCreateInfo vertStageInfo = vk::PipelineShaderStageCreateInfo()
			.setStage(vk::ShaderStageFlagBits::eVertex)
			.setModule(vs)
			// entrypoint (one shader module with multiple entry points for different shading stages?)
			.setPName("main")
			.setPSpecializationInfo(nullptr); // constants for optimization

		vk::PipelineShaderStageCreateInfo fragStageInfo = vk::PipelineShaderStageCreateInfo()
			.setStage(vk::ShaderStageFlagBits::eFragment) // fragment == pixel shader in hlsl
			.setModule(ps)
			.setPName("main")
			.setPSpecializationInfo(nullptr);
		vk::PipelineShaderStageCreateInfo shaderStages[] = { vertStageInfo, fragStageInfo };

		// Vertex Input descriptor
		vk::PipelineVertexInputStateCreateInfo vertexInputInfo = vk::PipelineVertexInputStateCreateInfo()
			.setVertexBindingDescriptionCount(0)
			.setVertexBindingDescriptions(nullptr)
			.setVertexAttributeDescriptionCount(0)
			.setVertexAttributeDescriptions(nullptr);

		// Input Assembly
		vk::PipelineInputAssemblyStateCreateInfo inputAssemplyInfo = vk::PipelineInputAssemblyStateCreateInfo()
			.setTopology(vk::PrimitiveTopology::eTriangleList)
			.setPrimitiveRestartEnable(VK_FALSE);

		// Scissor Rect
		vk::Rect2D scissorRect = vk::Rect2D()
			.setOffset({ 0, 0 })
			.setExtent(swapchainExtent);
		// Viewport
		vk::Viewport viewport = vk::Viewport()
			.setX(0.0f)
			.setY(0.0f)
			.setWidth(static_cast<float>(swapchainExtent.width))
			.setHeight(static_cast<float>(swapchainExtent.height))
			.setMinDepth(0.0f)
			.setMaxDepth(1.0f);

		// Viewport state creation (viewport + scissor rect)
		vk::PipelineViewportStateCreateInfo viewportStateInfo = vk::PipelineViewportStateCreateInfo()
			.setViewportCount(1)
			.setPViewports(&viewport)
			.setScissorCount(1)
			.setPScissors(&scissorRect);

		// Rasterizer
		vk::PipelineRasterizationStateCreateInfo rasterizerInfo = vk::PipelineRasterizationStateCreateInfo()
			.setDepthClampEnable(VK_FALSE)
			.setRasterizerDiscardEnable(VK_FALSE)
			.setPolygonMode(vk::PolygonMode::eFill)
			.setLineWidth(1.0f)
			.setCullMode(vk::CullModeFlagBits::eBack)
			.setFrontFace(vk::FrontFace::eClockwise)
			.setDepthBiasEnable(VK_FALSE)
			.setDepthBiasConstantFactor(0.0f)
			.setDepthBiasClamp(0.0f)
			.setDepthBiasSlopeFactor(0.0f);

		// Multisampling
		vk::PipelineMultisampleStateCreateInfo multisamplingInfo = vk::PipelineMultisampleStateCreateInfo()
			.setSampleShadingEnable(VK_FALSE)
			.setRasterizationSamples(vk::SampleCountFlagBits::e1)
			.setMinSampleShading(1.0f)
			.setPSampleMask(nullptr)
			.setAlphaToCoverageEnable(VK_FALSE)
			.setAlphaToOneEnable(VK_FALSE);

		// Color Blending
		// per-framebuffer
		vk::PipelineColorBlendAttachmentState colorBlendAttachment = vk::PipelineColorBlendAttachmentState()
			.setColorWriteMask(
				vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
				vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA)
			.setBlendEnable(VK_FALSE)
			.setSrcColorBlendFactor(vk::BlendFactor::eOne)
			.setDstColorBlendFactor(vk::BlendFactor::eZero)
			.setColorBlendOp(vk::BlendOp::eAdd)
			.setSrcAlphaBlendFactor(vk::BlendFactor::eOne)
			.setDstAlphaBlendFactor(vk::BlendFactor::eZero)
			.setAlphaBlendOp(vk::BlendOp::eAdd);
		// global
		vk::PipelineColorBlendStateCreateInfo colorBlendInfo = vk::PipelineColorBlendStateCreateInfo()
			.setLogicOpEnable(VK_FALSE)
			.setLogicOp(vk::LogicOp::eCopy)
			.setAttachmentCount(1)
			.setPAttachments(&colorBlendAttachment)
			.setBlendConstants({ 0.0f, 0.0f, 0.0f, 0.0f });

		// Pipeline Layout
		vk::PipelineLayoutCreateInfo pipelineLayoutInfo = vk::PipelineLayoutCreateInfo()
			.setSetLayoutCount(0)
			.setSetLayouts(nullptr)
			.setPushConstantRangeCount(0)
			.setPushConstantRanges(nullptr);
		pipelineLayout = device.createPipelineLayout(pipelineLayoutInfo);

		// Finally, create actual render pipeline
		vk::GraphicsPipelineCreateInfo graphicsPipelineInfo = vk::GraphicsPipelineCreateInfo()
			.setStageCount(2)
			.setPStages(shaderStages)
			// fixed-function stages
			.setPVertexInputState(&vertexInputInfo)
			.setPInputAssemblyState(&inputAssemplyInfo)
			.setPViewportState(&viewportStateInfo)
			.setPRasterizationState(&rasterizerInfo)
			.setPMultisampleState(&multisamplingInfo)
			.setPDepthStencilState(nullptr)
			.setPColorBlendState(&colorBlendInfo)
			.setPDynamicState(nullptr)
			// pipeline layout
			.setLayout(pipelineLayout)
			// render pass
			.setRenderPass(renderPass)
			.setSubpass(0) // index for the subpass this render pipeline will use
			// parent pipeline
			//.setFlags(vk::PipelineCreateFlagBits::eDerivative) // would be required for deriving from base pipelines
			.setBasePipelineHandle(nullptr)
			.setBasePipelineIndex(-1);

		vk::Result result;
		std::tie(result, graphicsPipeline) = device.createGraphicsPipeline(nullptr, graphicsPipelineInfo);
		switch (result)
		{
			case vk::Result::eSuccess: break;
			case vk::Result::ePipelineCompileRequiredEXT:
				VMI_LOG("Graphics pipeline creation: PipelineCompileRequiredEXT");
				break;
			default: assert(false);
		}
	}

	void create_framebuffers(DeviceManager& deviceManager)
	{
		swapchainFramebuffers.resize(swapchainImageViews.size());

		for (size_t i = 0; i < swapchainImageViews.size(); i++) {
			vk::FramebufferCreateInfo framebufferInfo = vk::FramebufferCreateInfo()
				.setRenderPass(renderPass)
				.setWidth(swapchainExtent.width)
				.setHeight(swapchainExtent.height)
				.setLayers(1)
				// attachments
				.setAttachmentCount(1)
				.setPAttachments(&swapchainImageViews[i]);

			swapchainFramebuffers[i] = deviceManager.get_logical_device().createFramebuffer(framebufferInfo);
		}
	}
	void create_command_pool(DeviceManager& deviceManager)
	{
		vk::CommandPoolCreateInfo commandPoolInfo = vk::CommandPoolCreateInfo()
			.setQueueFamilyIndex(deviceManager.get_device_wrapper().indices.iGraphicsFamily.value());
		//.setFlags(vk::CommandPoolCreateFlagBits::eTransient); Hint that command buffers are rerecorded with new commands very often

		commandPool = deviceManager.get_logical_device().createCommandPool(commandPoolInfo);
	}
	void create_command_buffers(DeviceManager& deviceManager)
	{
		commandBuffers.resize(swapchainFramebuffers.size());
		vk::CommandBufferAllocateInfo commandBufferInfo = vk::CommandBufferAllocateInfo()
			.setCommandPool(commandPool)
			.setLevel(vk::CommandBufferLevel::ePrimary) // secondary are used by primary command buffers for e.g. common operations
			.setCommandBufferCount(static_cast<uint32_t>(commandBuffers.size()));
		commandBuffers = deviceManager.get_logical_device().allocateCommandBuffers(commandBufferInfo);

		for (size_t i = 0; i < commandBuffers.size(); i++) {

			vk::CommandBuffer& commandBuffer = commandBuffers[i];
			vk::CommandBufferBeginInfo beginInfo = vk::CommandBufferBeginInfo()
				// eOneTimeSubmit: The command buffer will be rerecorded right after executing it once.
				// eRenderPassContinue: This is a secondary command buffer that will be entirely within a single render pass.
				// eSimultaneousUse: The command buffer can be resubmitted while it is also already pending execution.
				//.setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit)
				.setPInheritanceInfo(nullptr);
			commandBuffer.begin(beginInfo);

			std::array<float, 4> clearColor = { 0.0f, 0.0f, 0.0f, 1.0f };
			vk::ClearValue clearValue = vk::ClearValue().setColor(clearColor);

			vk::RenderPassBeginInfo renderPassBeginInfo = vk::RenderPassBeginInfo()
				.setRenderPass(renderPass)
				.setFramebuffer(swapchainFramebuffers[i])
				.setRenderArea(vk::Rect2D({ 0, 0 }, swapchainExtent))
				// clear value
				.setClearValueCount(1)
				.setPClearValues(&clearValue);


			commandBuffer.beginRenderPass(renderPassBeginInfo, vk::SubpassContents::eInline);
			commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, graphicsPipeline);
			commandBuffer.draw(3, 1, 0, 0);
			commandBuffer.endRenderPass();

			commandBuffer.end();
		}
	}

	void create_sync_objects(DeviceManager& deviceManager)
	{
		vk::Device& device = deviceManager.get_logical_device();

		imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
		renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
		inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);
		imagesInFlight.resize(swapchainImages.size(), nullptr);
		vk::SemaphoreCreateInfo semaphoreInfo = vk::SemaphoreCreateInfo();
		vk::FenceCreateInfo fenceInfo = vk::FenceCreateInfo().setFlags(vk::FenceCreateFlagBits::eSignaled);
		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
			imageAvailableSemaphores[i] = device.createSemaphore(semaphoreInfo);
			renderFinishedSemaphores[i] = device.createSemaphore(semaphoreInfo);
			inFlightFences[i] = device.createFence(fenceInfo);
		}
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
	vk::ShaderModule vs;
	vk::ShaderModule ps;
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