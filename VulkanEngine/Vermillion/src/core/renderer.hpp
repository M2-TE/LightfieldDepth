#pragma once

#include "vk_mem_alloc.hpp"
#include "ring_buffer.hpp"
#include "utils/types.hpp"
#include "wrappers/swapchain_wrapper.hpp"
#include "wrappers/shader_wrapper.hpp"
#include "wrappers/uniform_buffer_wrapper.hpp"
#include "geometry/indexed_geometry.hpp"

struct UniformBufferObject 
{
	glm::mat<4, 4, glm::f32, glm::packed_highp> model;
	glm::mat<4, 4, glm::f32, glm::packed_highp> view;
	glm::mat<4, 4, glm::f32, glm::packed_highp> proj;
};

class Renderer
{
public:
	Renderer() = default;
	~Renderer() = default;
	ROF_COPY_MOVE_DELETE(Renderer)

public:
	void init(DeviceWrapper& deviceWrapper, Window& window)
	{
		create_allocator(deviceWrapper, window);
		ringBuffer.init(nMaxFrames);

		create_descriptor_pools(deviceWrapper);
		create_command_pools(deviceWrapper);

		mvpBuffer.allocate(deviceWrapper, allocator, descPool, 0, vk::ShaderStageFlagBits::eVertex, nMaxFrames);
		geometry.allocate(allocator, transientCommandPool, deviceWrapper);

		create_shader_modules(deviceWrapper);
		create_KHR(deviceWrapper, window);

		imgui_init_vulkan(deviceWrapper, window);
		imgui_upload_fonts(deviceWrapper);
	}
	void destroy(DeviceWrapper& deviceWrapper)
	{
		vk::Device& device = deviceWrapper.logicalDevice;

		destroy_KHR(deviceWrapper);

		device.destroyShaderModule(vs);
		device.destroyShaderModule(ps);

		device.destroyCommandPool(transientCommandPool);
		device.destroyDescriptorPool(imguiDescPool);
		device.destroyDescriptorPool(descPool);

		ImGui_ImplVulkan_Shutdown();

		geometry.deallocate(allocator);
		mvpBuffer.deallocate(deviceWrapper, allocator);
		allocator.destroy();
	}
	void recreate_KHR(DeviceWrapper& deviceWrapper, Window& window) // TODO use better approach of recreating swapchain using old swapchain pointer
	{
		VMI_LOG("Rebuilding KHR");

		destroy_KHR(deviceWrapper);
		create_KHR(deviceWrapper, window);
	}

	void render(DeviceWrapper& deviceWrapper)
	{
		//VMI_LOG(ringBuffer.currentFrame);
		RingFrame& frameX = ringBuffer.frames[ringBuffer.currentFrame]; // tODO: selkect actual frame

		// TODO: the image available semaphore here is not strictly correct!
		vk::ResultValue imgResult = deviceWrapper.logicalDevice.acquireNextImageKHR(swapchainWrapper.swapchain, UINT64_MAX, frameX.imageAvailable);
		switch (imgResult.result) {
			case vk::Result::eSuccess: break;
			case vk::Result::eSuboptimalKHR: VMI_LOG("Suboptimal image acquisition."); break;
			case vk::Result::eErrorOutOfDateKHR: VMI_ERR("Swapchain: KHR out of date."); assert(false);
			default: assert(false);
		}

		// select the corresponding frame
		uint32_t iFrame = imgResult.value;
		RingFrame& frame = ringBuffer.frames[imgResult.value];

		// wait for fence of fetched frame before rendering to it
		vk::Result result = deviceWrapper.logicalDevice.waitForFences(frame.fence, VK_TRUE, UINT64_MAX);
		if (result != vk::Result::eSuccess) assert(false);

		// reset command pool and then record into it (using command buffer)
		deviceWrapper.logicalDevice.resetCommandPool(frame.commandPool);
		update_uniform_buffer(deviceWrapper, iFrame);
		record_command_buffer(iFrame);


		// Present
		{
			vk::PipelineStageFlags waitStages = vk::PipelineStageFlagBits::eColorAttachmentOutput;
			vk::SubmitInfo submitInfo = vk::SubmitInfo()
				.setPWaitDstStageMask(&waitStages)
				// semaphores
				.setWaitSemaphoreCount(1).setPWaitSemaphores(&frame.imageAvailable)
				.setSignalSemaphoreCount(1).setPSignalSemaphores(&frame.renderFinished)
				// command buffers
				.setCommandBufferCount(1).setPCommandBuffers(&frame.commandBuffer);

			deviceWrapper.logicalDevice.resetFences(frame.fence);
			deviceWrapper.queue.submit(submitInfo, frame.fence);

			vk::PresentInfoKHR presentInfo = vk::PresentInfoKHR()
				.setPImageIndices(&iFrame)
				// semaphores
				.setWaitSemaphoreCount(1).setPWaitSemaphores(&frame.renderFinished)
				// swapchains
				.setSwapchainCount(1).setPSwapchains(&swapchainWrapper.swapchain);
			result = deviceWrapper.queue.presentKHR(&presentInfo);
			if (result != vk::Result::eSuccess) assert(false);
		}

		ringBuffer.currentFrame = (ringBuffer.currentFrame + 1) % nMaxFrames;
	}

private:
	void create_allocator(DeviceWrapper& deviceWrapper, Window& window)
	{
		vma::AllocatorCreateInfo info = vma::AllocatorCreateInfo()
			.setPhysicalDevice(deviceWrapper.physicalDevice)
			.setDevice(deviceWrapper.logicalDevice)
			.setInstance(window.get_vulkan_instance())
			.setVulkanApiVersion(VK_API_VERSION_1_3);

		allocator = vma::createAllocator(info);
	}

	void create_KHR(DeviceWrapper& deviceWrapper, Window& window)
	{
		swapchainWrapper.init(deviceWrapper, window, nMaxFrames);

		create_render_pass(deviceWrapper);
		create_graphics_pipeline(deviceWrapper);

		ringBuffer.create_all(deviceWrapper, allocator, renderPass, swapchainWrapper);
	}
	void destroy_KHR(DeviceWrapper& deviceWrapper)
	{
		deviceWrapper.logicalDevice.destroyRenderPass(renderPass);
		deviceWrapper.logicalDevice.destroyPipeline(graphicsPipeline);
		deviceWrapper.logicalDevice.destroyPipelineLayout(pipelineLayout);

		ringBuffer.destroy_all(deviceWrapper, allocator);

		swapchainWrapper.destroy(deviceWrapper);
	}
	void create_render_pass(DeviceWrapper& deviceWrapper)
	{
		vk::AttachmentDescription colorAttachment = vk::AttachmentDescription()
			.setFormat(swapchainWrapper.surfaceFormat.format)
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

		vk::AttachmentDescription depthAttachment = vk::AttachmentDescription()
			.setFormat(vk::Format::eD24UnormS8Uint)
			.setSamples(vk::SampleCountFlagBits::e1)
			// depth
			.setLoadOp(vk::AttachmentLoadOp::eClear)
			.setStoreOp(vk::AttachmentStoreOp::eStore)
			// stencil
			.setStencilLoadOp(vk::AttachmentLoadOp::eClear)
			.setStencilStoreOp(vk::AttachmentStoreOp::eDontCare)
			.setInitialLayout(vk::ImageLayout::eUndefined)
			.setFinalLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal);
		vk::AttachmentReference depthAttachmentRef = vk::AttachmentReference()
			.setAttachment(1)
			.setLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal);

		vk::SubpassDescription subpassDesc = vk::SubpassDescription()
			.setPipelineBindPoint(vk::PipelineBindPoint::eGraphics)
			.setColorAttachmentCount(1).setPColorAttachments(&colorAttachmentRef)
			.setPDepthStencilAttachment(&depthAttachmentRef)
			// other:
			.setInputAttachmentCount(0).setPInputAttachments(nullptr)
			.setPreserveAttachmentCount(0).setPPreserveAttachments(nullptr)
			.setPResolveAttachments(nullptr);

		vk::SubpassDependency dependency = vk::SubpassDependency()
			.setDependencyFlags(vk::DependencyFlagBits::eByRegion) // for tiled GPUs.. i think.
			// src (when/what to wait on)
			.setSrcSubpass(VK_SUBPASS_EXTERNAL)
			.setSrcStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput | vk::PipelineStageFlagBits::eLateFragmentTests)
			.setSrcAccessMask(vk::AccessFlagBits::eNoneKHR)
			// dst (when/what to write to)
			.setDstSubpass(0)
			.setDstStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput | vk::PipelineStageFlagBits::eEarlyFragmentTests)
			.setDstAccessMask(vk::AccessFlagBits::eColorAttachmentWrite | vk::AccessFlagBits::eDepthStencilAttachmentWrite);

		std::array<vk::AttachmentDescription, 2> attachments = { colorAttachment, depthAttachment };
		vk::RenderPassCreateInfo renderPassInfo = vk::RenderPassCreateInfo()
			.setAttachmentCount(attachments.size()).setPAttachments(attachments.data())
			.setDependencyCount(1).setPDependencies(&dependency)
			.setSubpassCount(1).setPSubpasses(&subpassDesc);

		renderPass = deviceWrapper.logicalDevice.createRenderPass(renderPassInfo);
	}
	void create_graphics_pipeline(DeviceWrapper& deviceWrapper)
	{
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
		auto bindingDesc = Vertex::get_binding_desc();
		auto attrDesc = Vertex::get_attr_desc();
		vk::PipelineVertexInputStateCreateInfo vertexInputInfo = vk::PipelineVertexInputStateCreateInfo()
			.setVertexBindingDescriptionCount(1)
			.setVertexBindingDescriptions(bindingDesc)
			.setVertexAttributeDescriptionCount(static_cast<uint32_t>(attrDesc.size()))
			.setVertexAttributeDescriptions(attrDesc);

		// Input Assembly
		vk::PipelineInputAssemblyStateCreateInfo inputAssemplyInfo = vk::PipelineInputAssemblyStateCreateInfo()
			.setTopology(vk::PrimitiveTopology::eTriangleList)
			.setPrimitiveRestartEnable(VK_FALSE);

		// Scissor Rect
		vk::Rect2D scissorRect = vk::Rect2D()
			.setOffset({ 0, 0 })
			.setExtent(swapchainWrapper.extent);
		// Viewport
		vk::Viewport viewport = vk::Viewport()
			.setX(0.0f)
			.setY(0.0f)
			.setWidth(static_cast<float>(swapchainWrapper.extent.width))
			.setHeight(static_cast<float>(swapchainWrapper.extent.height))
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

		// Color Blending:
		
		// -> per-framebuffer
		vk::PipelineColorBlendAttachmentState colorBlendAttachment = vk::PipelineColorBlendAttachmentState()
			.setColorWriteMask(
				vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
				vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA) // theoretically no need to write transparency
			.setBlendEnable(VK_FALSE)
			// color blend
			.setSrcColorBlendFactor(vk::BlendFactor::eOne)
			.setDstColorBlendFactor(vk::BlendFactor::eZero)
			.setColorBlendOp(vk::BlendOp::eAdd)
			// alpha blend
			.setSrcAlphaBlendFactor(vk::BlendFactor::eOne)
			.setDstAlphaBlendFactor(vk::BlendFactor::eZero)
			.setAlphaBlendOp(vk::BlendOp::eAdd);
		// -> global
		vk::PipelineColorBlendStateCreateInfo colorBlendInfo = vk::PipelineColorBlendStateCreateInfo()
			.setLogicOpEnable(VK_FALSE)
			.setLogicOp(vk::LogicOp::eCopy)
			.setAttachmentCount(1)
			.setPAttachments(&colorBlendAttachment)
			.setBlendConstants({ 0.0f, 0.0f, 0.0f, 0.0f });

		vk::PipelineDepthStencilStateCreateInfo depthStencilInfo = vk::PipelineDepthStencilStateCreateInfo()
			.setPNext(nullptr)
			.setDepthTestEnable(VK_TRUE)
			.setDepthWriteEnable(VK_TRUE)
			.setDepthCompareOp(vk::CompareOp::eLessOrEqual)
			// Depth bounds
			.setDepthBoundsTestEnable(VK_FALSE)
			.setMinDepthBounds(0.0f)
			.setMaxDepthBounds(1.0f)
			// Stencil
			.setStencilTestEnable(VK_FALSE);

		// Pipeline Layout
		vk::PipelineLayoutCreateInfo pipelineLayoutInfo = vk::PipelineLayoutCreateInfo()
			.setSetLayoutCount(1)
			.setSetLayouts(mvpBuffer.get_desc_set_layout())
			.setPushConstantRangeCount(0)
			.setPushConstantRanges(nullptr);
		pipelineLayout = deviceWrapper.logicalDevice.createPipelineLayout(pipelineLayoutInfo);

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
			.setPDepthStencilState(&depthStencilInfo)
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
		std::tie(result, graphicsPipeline) = deviceWrapper.logicalDevice.createGraphicsPipeline(nullptr, graphicsPipelineInfo);
		switch (result)
		{
			case vk::Result::eSuccess: break;
			case vk::Result::ePipelineCompileRequiredEXT:
				VMI_LOG("Graphics pipeline creation: PipelineCompileRequiredEXT");
				break;
			default: assert(false);
		}
	}

	void create_shader_modules(DeviceWrapper& deviceWrapper)
	{
		vs = create_shader_module(deviceWrapper, shader_vs, shader_vs_size);
		ps = create_shader_module(deviceWrapper, shader_ps, shader_ps_size);
	}
	void create_descriptor_pools(DeviceWrapper& deviceWrapper)
	{
		// standard desc pool
		{
			static constexpr uint32_t poolSize = 1000;

			std::array<vk::DescriptorPoolSize, 1>  poolSizes =
			{
				vk::DescriptorPoolSize(vk::DescriptorType::eUniformBuffer, poolSize)
				// TODO: other stuff this pool will need
			};

			vk::DescriptorPoolCreateInfo info = vk::DescriptorPoolCreateInfo()
				.setFlags(vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet)
				.setMaxSets(poolSize * (uint32_t)poolSizes.size())
				.setPoolSizeCount((uint32_t)poolSizes.size())
				.setPPoolSizes(poolSizes.data());
			descPool = deviceWrapper.logicalDevice.createDescriptorPool(info);
		}

		// ImGui
		{
			uint32_t descCountImgui = 1000;
			std::array<vk::DescriptorPoolSize, 11> poolSizes =
			{
				vk::DescriptorPoolSize(vk::DescriptorType::eSampler, descCountImgui),
				vk::DescriptorPoolSize(vk::DescriptorType::eCombinedImageSampler, descCountImgui),
				vk::DescriptorPoolSize(vk::DescriptorType::eSampledImage, descCountImgui),
				vk::DescriptorPoolSize(vk::DescriptorType::eStorageImage, descCountImgui),
				vk::DescriptorPoolSize(vk::DescriptorType::eUniformTexelBuffer, descCountImgui),
				vk::DescriptorPoolSize(vk::DescriptorType::eStorageTexelBuffer, descCountImgui),
				vk::DescriptorPoolSize(vk::DescriptorType::eUniformBuffer, descCountImgui),
				vk::DescriptorPoolSize(vk::DescriptorType::eStorageBuffer, descCountImgui),
				vk::DescriptorPoolSize(vk::DescriptorType::eUniformBufferDynamic, descCountImgui),
				vk::DescriptorPoolSize(vk::DescriptorType::eStorageBufferDynamic, descCountImgui),
				vk::DescriptorPoolSize(vk::DescriptorType::eInputAttachment, descCountImgui)
			};

			vk::DescriptorPoolCreateInfo info = vk::DescriptorPoolCreateInfo()
				.setFlags(vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet)
				.setMaxSets(descCountImgui * (uint32_t)poolSizes.size())
				.setPoolSizeCount((uint32_t)poolSizes.size())
				.setPPoolSizes(poolSizes.data());

			imguiDescPool = deviceWrapper.logicalDevice.createDescriptorPool(info);
		}
	}
	void create_command_pools(DeviceWrapper& deviceWrapper)
	{
		vk::CommandPoolCreateInfo commandPoolInfo = vk::CommandPoolCreateInfo()
			.setQueueFamilyIndex(deviceWrapper.iQueue)
			.setFlags(vk::CommandPoolCreateFlagBits::eTransient);
		transientCommandPool = deviceWrapper.logicalDevice.createCommandPool(commandPoolInfo);
	}
	
	// ImGui
	void imgui_init_vulkan(DeviceWrapper& deviceWrapper, Window& window)
	{
		struct ImGui_ImplVulkan_InitInfo info = { 0 };
		info.Instance = window.get_vulkan_instance();
		info.PhysicalDevice = deviceWrapper.physicalDevice;
		info.Device = deviceWrapper.logicalDevice;
		info.QueueFamily = deviceWrapper.iQueue;
		info.Queue = deviceWrapper.queue;
		info.PipelineCache = pipelineCache;
		info.DescriptorPool = imguiDescPool;
		info.Subpass = 0;
		info.MinImageCount = (uint32_t) ringBuffer.frames.size(); // TODO: can prolly remove this one
		info.ImageCount = (uint32_t)ringBuffer.frames.size();
		info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;

		ImGui_ImplVulkan_Init(&info, renderPass);
	}
	void imgui_upload_fonts(DeviceWrapper& deviceWrapper)
	{
		vk::CommandBuffer commandBuffer = ringBuffer.frames[0].commandBuffer;
		deviceWrapper.logicalDevice.resetCommandPool(ringBuffer.frames[0].commandPool);

		vk::CommandBufferBeginInfo beginInfo = vk::CommandBufferBeginInfo()
			.setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
		commandBuffer.begin(beginInfo);

		// upload fonts
		ImGui_ImplVulkan_CreateFontsTexture(commandBuffer);

		commandBuffer.end();
		vk::SubmitInfo submitInfo = vk::SubmitInfo()
			.setCommandBufferCount(1)
			.setPCommandBuffers(&commandBuffer);
		deviceWrapper.queue.submit(submitInfo);

		deviceWrapper.logicalDevice.waitIdle();
		ImGui_ImplVulkan_DestroyFontUploadObjects();
	}

	// runtime
	void update_uniform_buffer(DeviceWrapper& deviceWrapper, uint32_t iCurrentFrame)
	{
		static auto startTime = std::chrono::high_resolution_clock::now();
		auto currentTime = std::chrono::high_resolution_clock::now();
		float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

		auto& ubo = mvpBuffer.data;
		ubo.model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
		ubo.view = glm::lookAt(glm::vec3(0.0f, 2.0f, -2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
		ubo.proj = glm::perspective(glm::radians(45.0f), (float)swapchainWrapper.extent.width / (float)swapchainWrapper.extent.height, 0.1f, 10.0f);

		mvpBuffer.update(iCurrentFrame);
	}
	void record_command_buffer(uint32_t iFrame)
	{
		vk::CommandBuffer& commandBuffer = ringBuffer.frames[iFrame].commandBuffer;
		vk::CommandBufferBeginInfo beginInfo = vk::CommandBufferBeginInfo()
			.setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit)
			.setPInheritanceInfo(nullptr);
		commandBuffer.begin(beginInfo);

		// clear color
		vk::ClearColorValue clearColorValue = vk::ClearColorValue()
			.setFloat32({ 0.0f, 0.0f, 0.0f, 1.0f });
		vk::ClearDepthStencilValue clearDepthStencilValue = vk::ClearDepthStencilValue()
			.setDepth(1.0f)
			.setStencil(0);
		std::array<vk::ClearValue, 2> clearValues = { vk::ClearValue(clearColorValue), vk::ClearValue(clearDepthStencilValue) };

		vk::RenderPassBeginInfo renderPassBeginInfo = vk::RenderPassBeginInfo()
			.setRenderPass(renderPass)
			.setFramebuffer(ringBuffer.frames[iFrame].swapchainFramebuffer)
			.setRenderArea(vk::Rect2D({ 0, 0 }, swapchainWrapper.extent))
			// clear value
			.setClearValueCount(clearValues.size()).setPClearValues(clearValues.data());

		commandBuffer.beginRenderPass(renderPassBeginInfo, vk::SubpassContents::eInline);
		commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, graphicsPipeline);

		commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelineLayout, 0, mvpBuffer.get_desc_set(iFrame), {});
		geometry.draw(commandBuffer);

		ImGui::Render();
		ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), commandBuffer);

		commandBuffer.endRenderPass();

		commandBuffer.end();
	}

private:
	uint32_t nMaxFrames = 2; // max frames in flight

	vma::Allocator allocator;
	SwapchainWrapper swapchainWrapper;
	RingBuffer ringBuffer;

	vk::ShaderModule vs, ps;
	vk::RenderPass renderPass;
	vk::Pipeline graphicsPipeline;
	vk::PipelineLayout pipelineLayout;
	vk::PipelineCache pipelineCache;

	vk::DescriptorPool imguiDescPool;
	vk::DescriptorPool descPool;

	vk::CommandPool transientCommandPool;

	UniformBufferWrapper<UniformBufferObject> mvpBuffer;
	IndexedGeometry geometry;
};