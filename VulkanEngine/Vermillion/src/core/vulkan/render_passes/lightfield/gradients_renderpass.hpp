#pragma once

struct GradientsRenderpassCreateInfo
{
	DeviceWrapper& deviceWrapper;
	SwapchainWrapper& swapchainWrapper;
	vma::Allocator& allocator;
	vk::DescriptorPool& descPool;
	Lightfield& lightfield;
};

class GradientsRenderpass
{
public:
	GradientsRenderpass() = default;
	~GradientsRenderpass() = default;
	ROF_COPY_MOVE_DELETE(GradientsRenderpass)
		 
public:
	void init(GradientsRenderpassCreateInfo& info)
	{
		create_shader_modules(info);
		create_render_pass(info);
		create_framebuffer(info);

		descSet = info.lightfield.descSetLightfield;
		descSetLayout = info.lightfield.descSetLayout;

		create_pipeline_layout(info);
		create_pipeline(info);

		fullscreenRect = vk::Rect2D({ 0, 0 }, info.swapchainWrapper.extent);
	}
	void destroy(DeviceWrapper& deviceWrapper)
	{
		auto& device = deviceWrapper.logicalDevice;

		// Shaders
		device.destroyShaderModule(vs);
		device.destroyShaderModule(ps);

		// Framebuffer
		device.destroyFramebuffer(framebuffer);

		// Render Pass
		deviceWrapper.logicalDevice.destroyRenderPass(renderPass);

		// Stages
		deviceWrapper.logicalDevice.destroyPipelineLayout(pipelineLayout);
		deviceWrapper.logicalDevice.destroyPipeline(graphicsPipeline);
	}

	void execute(vk::CommandBuffer& commandBuffer, PC pushConstant)
	{
		vk::RenderPassBeginInfo renderPassBeginInfo = vk::RenderPassBeginInfo()
			.setRenderPass(renderPass)
			.setFramebuffer(framebuffer)
			.setRenderArea(fullscreenRect);

		commandBuffer.beginRenderPass(renderPassBeginInfo, vk::SubpassContents::eInline);
		commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, graphicsPipeline);

		// draw fullscreen triangle
		commandBuffer.pushConstants<PC>(pipelineLayout, vk::ShaderStageFlagBits::eFragment, 0, pushConstant);

		commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelineLayout, 0, descSet, {});
		commandBuffer.draw(3, 1, 0, 0);

		commandBuffer.endRenderPass();
	}

private:
	void create_shader_modules(GradientsRenderpassCreateInfo& info)
	{
		vs = create_shader_module(info.deviceWrapper, lightfieldGradients.vs);
		ps = create_shader_module(info.deviceWrapper, lightfieldGradients.ps);
	}
	void create_render_pass(GradientsRenderpassCreateInfo& info)
	{
		std::array<vk::AttachmentDescription, 1> attachments = {
			// Output
			vk::AttachmentDescription()
				.setFormat(colorFormat)
				.setSamples(vk::SampleCountFlagBits::e1)
				.setLoadOp(vk::AttachmentLoadOp::eDontCare)
				.setStoreOp(vk::AttachmentStoreOp::eStore)
				.setStencilLoadOp(vk::AttachmentLoadOp::eDontCare)
				.setStencilStoreOp(vk::AttachmentStoreOp::eDontCare)
				.setInitialLayout(vk::ImageLayout::eUndefined)
				.setFinalLayout(vk::ImageLayout::eColorAttachmentOptimal)
		};

		// Subpass Descriptions
		vk::AttachmentReference output = vk::AttachmentReference(0, vk::ImageLayout::eColorAttachmentOptimal);
		vk::SubpassDescription subpass = vk::SubpassDescription()
			.setPipelineBindPoint(vk::PipelineBindPoint::eGraphics)
			.setPDepthStencilAttachment(nullptr)
			.setColorAttachments(output);

		// Subpass dependency
		vk::SubpassDependency dependency = vk::SubpassDependency()
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

		renderPass = info.deviceWrapper.logicalDevice.createRenderPass(renderPassInfo);
	}
	void create_framebuffer(GradientsRenderpassCreateInfo& info)
	{
		std::array<vk::ImageView, 1> attachments = {
			info.lightfield.gradientsImageView 
		};

		vk::FramebufferCreateInfo framebufferInfo = vk::FramebufferCreateInfo()
			.setRenderPass(renderPass)
			.setWidth(info.swapchainWrapper.extent.width)
			.setHeight(info.swapchainWrapper.extent.height)
			.setAttachments(attachments)
			.setLayers(1);

		framebuffer = info.deviceWrapper.logicalDevice.createFramebuffer(framebufferInfo);
	}

	void create_pipeline_layout(GradientsRenderpassCreateInfo& info)
	{
		vk::PushConstantRange pcr(vk::ShaderStageFlagBits::eFragment, 0, sizeof(PC));
		vk::PipelineLayoutCreateInfo pipelineLayoutInfo = vk::PipelineLayoutCreateInfo()
			.setSetLayouts(descSetLayout)
			.setPushConstantRanges(pcr);
		pipelineLayout = info.deviceWrapper.logicalDevice.createPipelineLayout(pipelineLayoutInfo);
	}
	void create_pipeline(GradientsRenderpassCreateInfo& info)
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
				.setExtent(info.swapchainWrapper.extent);
			// Viewport
			viewport = vk::Viewport()
				.setX(0.0f).setY(0.0f)
				.setMinDepth(0.0f).setMaxDepth(1.0f)
				.setWidth(static_cast<float>(info.swapchainWrapper.extent.width))
				.setHeight(static_cast<float>(info.swapchainWrapper.extent.height));

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

		auto result = info.deviceWrapper.logicalDevice.createGraphicsPipeline(pipelineCache, graphicsPipelineInfo);
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
	static constexpr vk::Format colorFormat = vk::Format::eR8G8B8A8Srgb;
	static constexpr uint32_t nCams = 9;
	vk::RenderPass renderPass;

	// subpasses
	vk::Pipeline graphicsPipeline;
	vk::PipelineLayout pipelineLayout;
	vk::PipelineCache pipelineCache; // TODO

	// shaders for the subpasses
	vk::ShaderModule vs, ps;

	// render resources
	vk::Framebuffer framebuffer;

	// desc layout
	vk::DescriptorSetLayout descSetLayout;
	vk::DescriptorSet descSet;

	// misc
	vk::Rect2D fullscreenRect;
};