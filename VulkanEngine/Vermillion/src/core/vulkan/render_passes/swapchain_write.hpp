#pragma once

class SwapchainWrite
{
public:
	SwapchainWrite() = default;
	~SwapchainWrite() = default;
	ROF_COPY_MOVE_DELETE(SwapchainWrite)

public:
	void init(DeviceWrapper& deviceWrapper, SwapchainWrapper& swapchainWrapper, vk::DescriptorPool& descPool, vk::ImageView& input)
	{
		create_shader_modules(deviceWrapper);
		create_render_pass(deviceWrapper, swapchainWrapper);
		create_framebuffer(deviceWrapper, swapchainWrapper, input);

		create_desc_set_layout(deviceWrapper);
		create_desc_set(deviceWrapper, descPool, input);

		create_pipeline_layout(deviceWrapper);
		create_pipeline(deviceWrapper, swapchainWrapper);

		fullscreenRect = vk::Rect2D({ 0, 0 }, swapchainWrapper.extent);
	}
	void destroy(DeviceWrapper& deviceWrapper)
	{
		auto& device = deviceWrapper.logicalDevice;

		// Shaders
		device.destroyShaderModule(vs);
		device.destroyShaderModule(ps);

		// Render Pass
		deviceWrapper.logicalDevice.destroyRenderPass(renderPass);
		for (size_t i = 0; i < framebuffers.size(); i++) {
			deviceWrapper.logicalDevice.destroyFramebuffer(framebuffers[i]);
		}

		// Stages
		deviceWrapper.logicalDevice.destroyPipelineLayout(pipelineLayout);
		deviceWrapper.logicalDevice.destroyPipeline(graphicsPipeline);

		// descriptors
		deviceWrapper.logicalDevice.destroyDescriptorSetLayout(descSetLayout);
	}

	void execute(vk::CommandBuffer& commandBuffer, uint32_t iFrame)
	{
		vk::RenderPassBeginInfo renderPassBeginInfo = vk::RenderPassBeginInfo()
			.setRenderPass(renderPass)
			.setFramebuffer(framebuffers[iFrame])
			.setRenderArea(fullscreenRect);

		commandBuffer.beginRenderPass(renderPassBeginInfo, vk::SubpassContents::eInline);
		commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, graphicsPipeline);

		// draw fullscreen triangle
		commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelineLayout, 0, descSet, {});
		commandBuffer.draw(3, 1, 0, 0);

		// write imgui ui to the output image
		ImGui::Render();
		ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), commandBuffer);

		commandBuffer.endRenderPass();
	}

	inline vk::RenderPass& get_render_pass()
	{
		return renderPass;
	}

private:
	void create_shader_modules(DeviceWrapper& deviceWrapper)
	{
		vs = create_shader_module(deviceWrapper, swapchainWrite.vs);
		ps = create_shader_module(deviceWrapper, swapchainWrite.ps);
	}
	void create_render_pass(DeviceWrapper& deviceWrapper, SwapchainWrapper& swapchainWrapper)
	{
		std::array<vk::AttachmentDescription, 2> attachments = {
			// Input
			vk::AttachmentDescription()
				.setFormat(vk::Format::eR8G8B8A8Srgb)
				.setSamples(vk::SampleCountFlagBits::e1)
				.setLoadOp(vk::AttachmentLoadOp::eLoad)
				.setStoreOp(vk::AttachmentStoreOp::eDontCare)
				.setStencilLoadOp(vk::AttachmentLoadOp::eDontCare)
				.setStencilStoreOp(vk::AttachmentStoreOp::eDontCare)
				.setInitialLayout(vk::ImageLayout::eColorAttachmentOptimal)
				.setFinalLayout(vk::ImageLayout::eShaderReadOnlyOptimal),
			// Output
			vk::AttachmentDescription()
				.setFormat(swapchainWrapper.surfaceFormat.format)
				.setSamples(vk::SampleCountFlagBits::e1)
				.setLoadOp(vk::AttachmentLoadOp::eDontCare)
				.setStoreOp(vk::AttachmentStoreOp::eStore)
				.setStencilLoadOp(vk::AttachmentLoadOp::eDontCare)
				.setStencilStoreOp(vk::AttachmentStoreOp::eDontCare)
				.setInitialLayout(vk::ImageLayout::eUndefined)
				.setFinalLayout(vk::ImageLayout::ePresentSrcKHR)
		};

		// Subpass Descriptions
		vk::AttachmentReference input = vk::AttachmentReference(0, vk::ImageLayout::eShaderReadOnlyOptimal);
		vk::AttachmentReference output = vk::AttachmentReference(1, vk::ImageLayout::eColorAttachmentOptimal);
		vk::SubpassDescription subpass = vk::SubpassDescription()
			.setPipelineBindPoint(vk::PipelineBindPoint::eGraphics)
			.setPDepthStencilAttachment(nullptr)
			.setInputAttachments(input).setColorAttachments(output)
			// misc other:
			.setPreserveAttachmentCount(0).setPPreserveAttachments(nullptr).setPResolveAttachments(nullptr);

		// Subpass dependency
		vk::SubpassDependency dependency = vk::SubpassDependency()
			.setDependencyFlags(vk::DependencyFlagBits::eByRegion)
			// src (when/what to wait on)
			.setSrcSubpass(VK_SUBPASS_EXTERNAL)
			.setSrcStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput)
			.setSrcAccessMask(vk::AccessFlagBits::eColorAttachmentRead)
			// dst (when/what to write to)
			.setDstSubpass(0)
			.setDstStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput)
			.setDstAccessMask(vk::AccessFlagBits::eColorAttachmentWrite);

		vk::RenderPassCreateInfo renderPassInfo = vk::RenderPassCreateInfo()
			.setAttachments(attachments)
			.setDependencies(dependency)
			.setSubpasses(subpass);

		renderPass = deviceWrapper.logicalDevice.createRenderPass(renderPassInfo);
	}
	void create_framebuffer(DeviceWrapper& deviceWrapper, SwapchainWrapper& swapchainWrapper, vk::ImageView& input)
	{
		std::array<vk::ImageView, 2> attachments = { input, VK_NULL_HANDLE };

		// create one framebuffer for each potential image view output
		framebuffers.resize(swapchainWrapper.images.size());
		for (size_t i = 0; i < swapchainWrapper.images.size(); i++) {

			attachments[1] = swapchainWrapper.imageViews[i];

			vk::FramebufferCreateInfo framebufferInfo = vk::FramebufferCreateInfo()
				.setRenderPass(renderPass)
				.setWidth(swapchainWrapper.extent.width)
				.setHeight(swapchainWrapper.extent.height)
				.setAttachments(attachments)
				.setLayers(1);

			framebuffers[i] = deviceWrapper.logicalDevice.createFramebuffer(framebufferInfo);
		}
	}

	void create_desc_set_layout(DeviceWrapper& deviceWrapper)
	{
		vk::DescriptorSetLayoutBinding setLayoutBinding = vk::DescriptorSetLayoutBinding()
			.setBinding(0)
			.setDescriptorCount(1)
			.setDescriptorType(vk::DescriptorType::eInputAttachment)
			.setStageFlags(vk::ShaderStageFlagBits::eFragment);

		// create descriptor set layout from the bindings
		vk::DescriptorSetLayoutCreateInfo createInfo = vk::DescriptorSetLayoutCreateInfo()
			.setBindings(setLayoutBinding);

		descSetLayout = deviceWrapper.logicalDevice.createDescriptorSetLayout(createInfo);
	}
	void create_desc_set(DeviceWrapper& deviceWrapper, vk::DescriptorPool& descPool, vk::ImageView& imageView)
	{
		// allocate the descriptor sets using descriptor pool
		vk::DescriptorSetAllocateInfo allocInfo = vk::DescriptorSetAllocateInfo()
			.setDescriptorPool(descPool)
			.setDescriptorSetCount(1).setPSetLayouts(&descSetLayout);
		descSet = deviceWrapper.logicalDevice.allocateDescriptorSets(allocInfo)[0];

		vk::DescriptorImageInfo descriptor = vk::DescriptorImageInfo()
			.setImageLayout(vk::ImageLayout::eShaderReadOnlyOptimal)
			.setImageView(imageView)
			.setSampler(nullptr);

		// input image
		vk::WriteDescriptorSet descBufferWrites = vk::WriteDescriptorSet()
			.setDstSet(descSet)
			.setDstBinding(0)
			.setDstArrayElement(0)
			.setDescriptorType(vk::DescriptorType::eInputAttachment)
			//
			.setPBufferInfo(nullptr)
			.setImageInfo(descriptor)
			.setPTexelBufferView(nullptr);

		deviceWrapper.logicalDevice.updateDescriptorSets(descBufferWrites, {});
	}

	void create_pipeline_layout(DeviceWrapper& deviceWrapper)
	{
		vk::PipelineLayoutCreateInfo pipelineLayoutInfo = vk::PipelineLayoutCreateInfo()
			.setSetLayouts(descSetLayout)
			.setPushConstantRangeCount(0).setPushConstantRanges(nullptr);
		pipelineLayout = deviceWrapper.logicalDevice.createPipelineLayout(pipelineLayoutInfo);
	}
	void create_pipeline(DeviceWrapper& deviceWrapper, SwapchainWrapper& swapchainWrapper)
	{
		// Shaders
		std::array<vk::PipelineShaderStageCreateInfo, 2> shaderStages;
		{
			shaderStages[0] = vk::PipelineShaderStageCreateInfo()
				.setStage(vk::ShaderStageFlagBits::eVertex)
				.setModule(vs)
				// entrypoint (one shader module with multiple entry points for different shading stages?)
				.setPName("main")
				.setPSpecializationInfo(nullptr); // constants for optimization

			shaderStages[1] = vk::PipelineShaderStageCreateInfo()
				.setStage(vk::ShaderStageFlagBits::eFragment)
				.setModule(ps)
				.setPName("main")
				.setPSpecializationInfo(nullptr);
		}

		// Input
		vk::PipelineVertexInputStateCreateInfo vertexInputInfo;
		vk::PipelineInputAssemblyStateCreateInfo inputAssemplyInfo;
		{
			// Vertex Input descriptor
			vertexInputInfo = vk::PipelineVertexInputStateCreateInfo()
				.setVertexBindingDescriptionCount(0).setVertexBindingDescriptions(nullptr)
				.setVertexAttributeDescriptionCount(0).setVertexAttributeDescriptions(nullptr);

			// Input Assembly
			inputAssemplyInfo = vk::PipelineInputAssemblyStateCreateInfo()
				.setTopology(vk::PrimitiveTopology::eTriangleList)
				.setPrimitiveRestartEnable(VK_FALSE);
		}

		// Viewport
		vk::Rect2D scissorRect;
		vk::Viewport viewport;
		vk::PipelineViewportStateCreateInfo viewportStateInfo;
		{
			// Scissor Rect
			scissorRect = vk::Rect2D()
				.setOffset({ 0, 0 })
				.setExtent(swapchainWrapper.extent);
			// Viewport
			viewport = vk::Viewport()
				.setX(0.0f).setY(0.0f)
				.setMinDepth(0.0f).setMaxDepth(1.0f)
				.setWidth(static_cast<float>(swapchainWrapper.extent.width))
				.setHeight(static_cast<float>(swapchainWrapper.extent.height));

			// Viewport state creation
			viewportStateInfo = vk::PipelineViewportStateCreateInfo()
				.setViewportCount(1).setPViewports(&viewport)
				.setScissorCount(1).setPScissors(&scissorRect);
		}

		// Rasterization and Multisampling
		vk::PipelineRasterizationStateCreateInfo rasterizerInfo;
		vk::PipelineMultisampleStateCreateInfo multisamplingInfo;
		{
			// Rasterizer
			rasterizerInfo = vk::PipelineRasterizationStateCreateInfo()
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
			multisamplingInfo = vk::PipelineMultisampleStateCreateInfo()
				.setSampleShadingEnable(VK_FALSE)
				.setRasterizationSamples(vk::SampleCountFlagBits::e1)
				.setMinSampleShading(1.0f)
				.setPSampleMask(nullptr)
				.setAlphaToCoverageEnable(VK_FALSE)
				.setAlphaToOneEnable(VK_FALSE);
		}

		// Color Blending
		vk::PipelineColorBlendAttachmentState colorBlendAttachment;
		vk::PipelineColorBlendStateCreateInfo colorBlendInfo;
		{
			// final output image
			colorBlendAttachment = vk::PipelineColorBlendAttachmentState()
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
			colorBlendInfo = vk::PipelineColorBlendStateCreateInfo()
				.setLogicOpEnable(VK_FALSE).setLogicOp(vk::LogicOp::eCopy)
				.setAttachments(colorBlendAttachment)
				.setBlendConstants({ 0.0f, 0.0f, 0.0f, 0.0f });
		}

		// Depth Stencil
		vk::PipelineDepthStencilStateCreateInfo depthStencilInfo;
		{
			depthStencilInfo = vk::PipelineDepthStencilStateCreateInfo()
				.setDepthTestEnable(VK_FALSE)
				.setDepthWriteEnable(VK_FALSE)
				.setDepthCompareOp(vk::CompareOp::eLessOrEqual)
				// Depth bounds
				.setDepthBoundsTestEnable(VK_FALSE)
				.setMinDepthBounds(0.0f).setMaxDepthBounds(1.0f)
				// Stencil
				.setStencilTestEnable(VK_FALSE);
		}

		// Finally, create actual render pipeline
		vk::GraphicsPipelineCreateInfo graphicsPipelineInfo = vk::GraphicsPipelineCreateInfo()
			.setStageCount((uint32_t)shaderStages.size())
			.setPStages(shaderStages.data())
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
			.setSubpass(0);

		auto result = deviceWrapper.logicalDevice.createGraphicsPipeline(pipelineCache, graphicsPipelineInfo);
		switch (result.result)
		{
			case vk::Result::eSuccess: break;
			case vk::Result::ePipelineCompileRequiredEXT:
				VMI_LOG("Graphics pipeline creation: PipelineCompileRequiredEXT");
				break;
			default: assert(false);
		}
		graphicsPipeline = result.value;
	}
	
private:
	vk::RenderPass renderPass;
	std::vector<vk::Framebuffer> framebuffers;
	vk::ShaderModule vs, ps;

	// subpasses
	vk::Pipeline graphicsPipeline;
	vk::PipelineLayout pipelineLayout;
	vk::PipelineCache pipelineCache; // TODO

	// descriptor
	vk::DescriptorSetLayout descSetLayout;
	vk::DescriptorSet descSet;

	// misc
	vk::Rect2D fullscreenRect;
	vk::ClearValue clearValue;
};