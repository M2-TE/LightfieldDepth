#pragma once

#include "wrappers/swapchain_wrapper.hpp"
#include "wrappers/shader_wrapper.hpp"

struct Vertex
{
	glm::vec<4, float, glm::packed_highp> pos;
	glm::vec<4, float, glm::packed_highp> col;

	static vk::VertexInputBindingDescription get_binding_desc() {
		vk::VertexInputBindingDescription desc = vk::VertexInputBindingDescription()
			.setBinding(0)
			.setStride(sizeof(Vertex))
			.setInputRate(vk::VertexInputRate::eVertex);
		return desc;
	}
	static std::array<vk::VertexInputAttributeDescription, 2> get_attr_desc() {
		std::array<vk::VertexInputAttributeDescription, 2> desc;
		desc[0] = vk::VertexInputAttributeDescription()
			.setBinding(0)
			.setLocation(0)
			.setFormat(vk::Format::eR32G32B32A32Sfloat)
			.setOffset(offsetof(Vertex, pos));
		desc[1] = vk::VertexInputAttributeDescription()
			.setBinding(0)
			.setLocation(1)
			.setFormat(vk::Format::eR32G32B32A32Sfloat)
			.setOffset(offsetof(Vertex, col));
		return desc;
	}
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
		create_shader_modules(deviceWrapper);

		swapchainWrapper.init(deviceWrapper, window);
		create_render_pass(deviceWrapper);
		create_graphics_pipeline(deviceWrapper);
		swapchainWrapper.create_framebuffers(deviceWrapper, renderPass);

		create_descriptor_pools(deviceWrapper);
		create_command_pools(deviceWrapper);
		create_command_buffers(deviceWrapper);

		create_vertex_buffer(deviceWrapper);
		create_index_buffer(deviceWrapper);

		create_sync_objects(deviceWrapper);

		imgui_init_vulkan(deviceWrapper, window);
		imgui_upload_fonts(deviceWrapper);
	}
	void recreate_KHR(DeviceWrapper& deviceWrapper, Window& window) // TODO use better approach of recreating swapchain using old swapchain pointer
	{
		VMI_LOG("Rebuilding KHR");

		destroy_KHR(deviceWrapper);

		swapchainWrapper.init(deviceWrapper, window);
		create_render_pass(deviceWrapper);
		create_graphics_pipeline(deviceWrapper);
		swapchainWrapper.create_framebuffers(deviceWrapper, renderPass);
	}
	void destroy_KHR(DeviceWrapper& deviceWrapper)
	{
		swapchainWrapper.destroy(deviceWrapper);

		deviceWrapper.logicalDevice.destroyPipeline(graphicsPipeline);
		deviceWrapper.logicalDevice.destroyPipelineLayout(pipelineLayout);
		deviceWrapper.logicalDevice.destroyRenderPass(renderPass);
	}
	void destroy(DeviceWrapper& deviceWrapper)
	{
		vk::Device& device = deviceWrapper.logicalDevice;

		destroy_KHR(deviceWrapper);

		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
			device.destroySemaphore(imageAvailableSemaphores[i]);
			device.destroySemaphore(renderFinishedSemaphores[i]);
			device.destroyFence(inFlightFences[i]);
		}

		device.destroyShaderModule(vs);
		device.destroyShaderModule(ps);

		for (uint32_t i = 0; i < commandPools.size(); i++) {
			device.destroyCommandPool(commandPools[i]);
		}
		device.destroyCommandPool(transientCommandPool);
		device.destroyDescriptorPool(imguiDescPool);

		device.destroyBuffer(vertexBuffer);
		device.freeMemory(vertexBufferMemory);
		device.destroyBuffer(indexBuffer);
		device.freeMemory(indexBufferMemory);

		ImGui_ImplVulkan_Shutdown();
	}

	void render(DeviceWrapper& deviceWrapper)
	{
		vk::Device& device = deviceWrapper.logicalDevice;

		// wait for fence of current frame before going any further
		vk::Result result = device.waitForFences(inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);
		if (result != vk::Result::eSuccess) assert(false);

		vk::ResultValue imgResult = device.acquireNextImageKHR(swapchainWrapper.swapchain, UINT64_MAX, imageAvailableSemaphores[currentFrame], nullptr);
		switch (imgResult.result) {
			case vk::Result::eSuccess: break;
			case vk::Result::eNotReady: VMI_LOG("Images not ready."); return;
			case vk::Result::eSuboptimalKHR: VMI_LOG("Suboptimal image acquisition."); break;
			case vk::Result::eErrorOutOfDateKHR: VMI_LOG("Swapchain: KHR out of date."); return;
			default: assert(false);
		}

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
			.setPCommandBuffers(&commandBuffers[currentFrame]);

		device.resetCommandPool(commandPools[currentFrame]);
		record_command_buffer(imgResult.value, currentFrame);

		device.resetFences(inFlightFences[currentFrame]); // FENCE
		deviceWrapper.queue.submit(submitInfo, inFlightFences[currentFrame]);


		vk::PresentInfoKHR presentInfo = vk::PresentInfoKHR()
			.setPImageIndices(&imgResult.value)
			// semaphores
			.setWaitSemaphoreCount(1)
			.setPWaitSemaphores(&renderFinishedSemaphores[currentFrame])
			// swapchains
			.setSwapchainCount(1)
			.setPSwapchains(&swapchainWrapper.swapchain);
			//.setPResults(nullptr); // optional if theres multiple swapchains
		result = deviceWrapper.queue.presentKHR(&presentInfo);
		//if (result != vk::Result::eSuccess) assert(false);

		// advance frame index
		currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
	}

private:
	void create_shader_modules(DeviceWrapper& deviceWrapper)
	{
		vs = create_shader_module(deviceWrapper, shader_vs, shader_vs_size);
		ps = create_shader_module(deviceWrapper, shader_ps, shader_ps_size);
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
				vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA)
			.setBlendEnable(VK_FALSE)
			.setSrcColorBlendFactor(vk::BlendFactor::eOne)
			.setDstColorBlendFactor(vk::BlendFactor::eZero)
			.setColorBlendOp(vk::BlendOp::eAdd)
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

		// Pipeline Layout
		vk::PipelineLayoutCreateInfo pipelineLayoutInfo = vk::PipelineLayoutCreateInfo()
			.setSetLayoutCount(0)
			.setSetLayouts(nullptr)
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

	void create_command_pools(DeviceWrapper& deviceWrapper)
	{
		vk::CommandPoolCreateInfo commandPoolInfo;

		commandPoolInfo = vk::CommandPoolCreateInfo()
			.setQueueFamilyIndex(deviceWrapper.iQueue);
		commandPools.resize(MAX_FRAMES_IN_FLIGHT);
		for (uint32_t i = 0; i < commandPools.size(); i++) {
			commandPools[i] = deviceWrapper.logicalDevice.createCommandPool(commandPoolInfo);
		}

		commandPoolInfo = vk::CommandPoolCreateInfo()
			.setQueueFamilyIndex(deviceWrapper.iQueue)
			.setFlags(vk::CommandPoolCreateFlagBits::eTransient);
		transientCommandPool = deviceWrapper.logicalDevice.createCommandPool(commandPoolInfo);
	}
	void create_command_buffers(DeviceWrapper& deviceWrapper)
	{
		commandBuffers.resize(MAX_FRAMES_IN_FLIGHT);
		for (uint32_t i = 0; i < commandPools.size(); i++) {
			vk::CommandBufferAllocateInfo commandBufferInfo = vk::CommandBufferAllocateInfo()
				.setCommandPool(commandPools[i])
				.setLevel(vk::CommandBufferLevel::ePrimary) // secondary are used by primary command buffers for e.g. common operations
				.setCommandBufferCount(1);
			commandBuffers[i] = deviceWrapper.logicalDevice.allocateCommandBuffers(commandBufferInfo)[0];
		}
	}
	void create_descriptor_pools(DeviceWrapper& deviceWrapper)
	{
		// do stuff
		vk::DescriptorPoolSize pool_sizes[] =
		{
			vk::DescriptorPoolSize(vk::DescriptorType::eSampler, 1000),
			vk::DescriptorPoolSize(vk::DescriptorType::eCombinedImageSampler, 1000),
			vk::DescriptorPoolSize(vk::DescriptorType::eSampledImage, 1000),
			vk::DescriptorPoolSize(vk::DescriptorType::eStorageImage, 1000),
			vk::DescriptorPoolSize(vk::DescriptorType::eUniformTexelBuffer, 1000),
			vk::DescriptorPoolSize(vk::DescriptorType::eStorageTexelBuffer, 1000),
			vk::DescriptorPoolSize(vk::DescriptorType::eUniformBuffer, 1000),
			vk::DescriptorPoolSize(vk::DescriptorType::eStorageBuffer, 1000),
			vk::DescriptorPoolSize(vk::DescriptorType::eUniformBufferDynamic, 1000),
			vk::DescriptorPoolSize(vk::DescriptorType::eStorageBufferDynamic, 1000),
			vk::DescriptorPoolSize(vk::DescriptorType::eInputAttachment, 1000),
		};

		vk::DescriptorPoolCreateInfo info = vk::DescriptorPoolCreateInfo()
			.setFlags(vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet)
			.setMaxSets(1000 * IM_ARRAYSIZE(pool_sizes))
			.setPoolSizeCount((uint32_t)IM_ARRAYSIZE(pool_sizes))
			.setPPoolSizes(pool_sizes);

		imguiDescPool = deviceWrapper.logicalDevice.createDescriptorPool(info);
	}
	
	uint32_t find_memory_type(DeviceWrapper& deviceWrapper, uint32_t typeFilter, vk::MemoryPropertyFlags properties)
	{
		for (uint32_t i = 0; i < deviceWrapper.deviceMemProperties.memoryTypeCount; i++) {
			if (typeFilter & (1 << i) && (deviceWrapper.deviceMemProperties.memoryTypes[i].propertyFlags & properties) == properties)
			{
				return i;
			}
		}
		throw std::runtime_error("failed to find suitable memory type!");
	}
	// TODO: dont use vk::allocateMemory for every individual buffer. make one large one and work with offsets!
	void create_buffer(DeviceWrapper& deviceWrapper, vk::Buffer& buffer, vk::DeviceMemory& bufferMemory, vk::DeviceSize size, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags properties)
	{
		vk::BufferCreateInfo bufferInfo = vk::BufferCreateInfo()
			.setSize(size)
			.setUsage(usage)
			.setSharingMode(vk::SharingMode::eExclusive);
		buffer = deviceWrapper.logicalDevice.createBuffer(bufferInfo, nullptr);

		vk::MemoryRequirements memReqs;
		memReqs = deviceWrapper.logicalDevice.getBufferMemoryRequirements(buffer);

		vk::MemoryAllocateInfo allocInfo = vk::MemoryAllocateInfo()
			.setAllocationSize(memReqs.size)
			.setMemoryTypeIndex(find_memory_type(deviceWrapper, memReqs.memoryTypeBits, properties));
		bufferMemory = deviceWrapper.logicalDevice.allocateMemory(allocInfo);
		deviceWrapper.logicalDevice.bindBufferMemory(buffer, bufferMemory, 0);
	}
	void copy_buffer(DeviceWrapper& deviceWrapper, vk::Buffer src, vk::Buffer dst, vk::DeviceSize size)
	{
		vk::CommandBufferAllocateInfo allocInfo = vk::CommandBufferAllocateInfo()
			.setLevel(vk::CommandBufferLevel::ePrimary)
			.setCommandPool(transientCommandPool)
			.setCommandBufferCount(1);

		vk::CommandBuffer commandBuffer;
		auto res = deviceWrapper.logicalDevice.allocateCommandBuffers(&allocInfo, &commandBuffer);

		// begin recording to temporary command buffer
		vk::CommandBufferBeginInfo beginInfo = vk::CommandBufferBeginInfo()
			.setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
		commandBuffer.begin(beginInfo);

		vk::BufferCopy copyRegion = vk::BufferCopy()
			.setSrcOffset(0)
			.setDstOffset(0)
			.setSize(size);
		commandBuffer.copyBuffer(src, dst, copyRegion);
		commandBuffer.end();

		vk::SubmitInfo submitInfo = vk::SubmitInfo()
			.setCommandBufferCount(1)
			.setPCommandBuffers(&commandBuffer);
		deviceWrapper.queue.submit(submitInfo);
		deviceWrapper.queue.waitIdle(); // TODO: change this to wait on a fence instead (upon queue submit) so multiple memory transfers would be possible

		// free command buffer directly after use
		deviceWrapper.logicalDevice.freeCommandBuffers(transientCommandPool, commandBuffer);
	}
	void create_vertex_buffer(DeviceWrapper& deviceWrapper)
	{
		vk::DeviceSize bufferSize = sizeof(Vertex) * vertices.size();
		vk::Buffer stagingBuffer;
		vk::DeviceMemory stagingBufferMemory;

		create_buffer(deviceWrapper, stagingBuffer, stagingBufferMemory, bufferSize,
			vk::BufferUsageFlagBits::eTransferSrc,
			vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);

		void* data = deviceWrapper.logicalDevice.mapMemory(stagingBufferMemory, 0, VK_WHOLE_SIZE);
		memcpy(data, vertices.data(), static_cast<size_t>(bufferSize));
		deviceWrapper.logicalDevice.unmapMemory(stagingBufferMemory);

		create_buffer(deviceWrapper, vertexBuffer, vertexBufferMemory, bufferSize,
			vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eVertexBuffer,
			vk::MemoryPropertyFlagBits::eDeviceLocal);

		copy_buffer(deviceWrapper, stagingBuffer, vertexBuffer, bufferSize);

		// free up staging buffer
		deviceWrapper.logicalDevice.destroyBuffer(stagingBuffer);
		deviceWrapper.logicalDevice.freeMemory(stagingBufferMemory);
	}
	void create_index_buffer(DeviceWrapper& deviceWrapper)
	{
		vk::DeviceSize bufferSize = sizeof(Index) * indices.size();

		vk::Buffer stagingBuffer;
		vk::DeviceMemory stagingBufferMemory;

		create_buffer(deviceWrapper, stagingBuffer, stagingBufferMemory, bufferSize,
			vk::BufferUsageFlagBits::eTransferSrc,
			vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);

		void* data = deviceWrapper.logicalDevice.mapMemory(stagingBufferMemory, 0, VK_WHOLE_SIZE);
		memcpy(data, indices.data(), static_cast<size_t>(bufferSize));
		deviceWrapper.logicalDevice.unmapMemory(stagingBufferMemory);

		create_buffer(deviceWrapper, indexBuffer, indexBufferMemory, bufferSize,
			vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eIndexBuffer,
			vk::MemoryPropertyFlagBits::eDeviceLocal);

		copy_buffer(deviceWrapper, stagingBuffer, indexBuffer, bufferSize);

		// free up staging buffer
		deviceWrapper.logicalDevice.destroyBuffer(stagingBuffer);
		deviceWrapper.logicalDevice.freeMemory(stagingBufferMemory);
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
		info.MinImageCount = (uint32_t) swapchainWrapper.images.size();
		info.ImageCount = (uint32_t) swapchainWrapper.images.size();
		info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;

		ImGui_ImplVulkan_Init(&info, renderPass);
	}
	void imgui_upload_fonts(DeviceWrapper& deviceWrapper)
	{
		vk::CommandBuffer commandBuffer = commandBuffers[currentFrame];
		deviceWrapper.logicalDevice.resetCommandPool(commandPools[currentFrame]);

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
	void record_command_buffer(uint32_t iFrameBuffer, uint32_t iCommandBuffer)
	{
		vk::CommandBuffer& commandBuffer = commandBuffers[iCommandBuffer];


		vk::CommandBufferBeginInfo beginInfo = vk::CommandBufferBeginInfo()
			// eOneTimeSubmit: The command buffer will be rerecorded right after executing it once.
			// eRenderPassContinue: This is a secondary command buffer that will be entirely within a single render pass.
			// eSimultaneousUse: The command buffer can be resubmitted while it is also already pending execution.
			.setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit)
			.setPInheritanceInfo(nullptr);
		commandBuffer.begin(beginInfo);

		std::array<float, 4> clearColor = { 0.0f, 0.0f, 0.0f, 1.0f };
		vk::ClearValue clearValue = vk::ClearValue().setColor(clearColor);

		vk::RenderPassBeginInfo renderPassBeginInfo = vk::RenderPassBeginInfo()
			.setRenderPass(renderPass)
			.setFramebuffer(swapchainWrapper.framebuffers[iFrameBuffer])
			.setRenderArea(vk::Rect2D({ 0, 0 }, swapchainWrapper.extent))
			// clear value
			.setClearValueCount(1)
			.setPClearValues(&clearValue);

		commandBuffer.beginRenderPass(renderPassBeginInfo, vk::SubpassContents::eInline);
		commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, graphicsPipeline);

		vk::Buffer vertexBuffers[] = { vertexBuffer };
		vk::DeviceSize offsets[] = { 0 };
		commandBuffer.bindVertexBuffers(0, 1, vertexBuffers, offsets);
		commandBuffer.bindIndexBuffer(indexBuffer, 0, vk::IndexType::eUint16);
		commandBuffer.drawIndexed(static_cast<uint32_t>(indices.size()), 1, 0, 0, 0);


		ImGui::Render();
		ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), commandBuffer);

		commandBuffer.endRenderPass();

		commandBuffer.end();
	}

private:
	// lazy constants (settings?)
	static constexpr int32_t MAX_FRAMES_IN_FLIGHT = 2;
	static constexpr vk::Format targetFormat = vk::Format::eR8G8B8A8Srgb;
	static constexpr vk::ColorSpaceKHR targetColorSpace = vk::ColorSpaceKHR::eSrgbNonlinear;
	static constexpr vk::PresentModeKHR targetPresentMode = vk::PresentModeKHR::eFifo; // vsync
	//static constexpr vk::PresentModeKHR targetPresentMode = vk::PresentModeKHR::eMailbox;

	SwapchainWrapper swapchainWrapper;

	vk::ShaderModule vs, ps;
	vk::RenderPass renderPass;
	vk::Pipeline graphicsPipeline;
	vk::PipelineLayout pipelineLayout;
	vk::PipelineCache pipelineCache;
	vk::DescriptorPool imguiDescPool;

	// TODO: make number of images in swapchain based on (min + max_frames_etc - 1)
	//vk::CommandPool commandPool;
	vk::CommandPool transientCommandPool;
	std::vector<vk::CommandPool> commandPools;
	std::vector<vk::CommandBuffer> commandBuffers;

	std::vector<vk::Semaphore> imageAvailableSemaphores;
	std::vector<vk::Semaphore> renderFinishedSemaphores;
	std::vector<vk::Fence> inFlightFences;
	uint32_t currentFrame = 0;

	// TODO: encapsulate this
	// also TODO: store both index/vertex data in the same buffer and use offsets to access each
	vk::Buffer vertexBuffer;
	vk::Buffer indexBuffer;
	vk::DeviceMemory vertexBufferMemory;
	vk::DeviceMemory indexBufferMemory;
	std::vector<Vertex> vertices = { {
			{{-0.5f, -0.5f, 0.0f, 1.0f}, {1.0f, 0.0f, 0.0f, 1.0f}},
			{{0.5f, -0.5f, 0.0f, 1.0f}, {0.0f, 1.0f, 0.0f, 1.0f}},
			{{0.5f, 0.5f, 0.0f, 1.0f}, {0.0f, 0.0f, 1.0f, 1.0f}},
			{{-0.5f, 0.5f, 0.0f, 1.0f}, {1.0f, 1.0f, 0.0f, 1.0f}}
		}
	};
	typedef uint16_t Index;
	std::vector<Index> indices = {
		0, 1, 2, 2, 3, 0
	};
};