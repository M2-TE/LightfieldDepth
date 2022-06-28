#pragma once

struct DisparityRenderpassCreateInfo
{
	DeviceWrapper& deviceWrapper;
	SwapchainWrapper& swapchainWrapper;
	vma::Allocator& allocator;
	vk::DescriptorPool& descPool;
	Lightfield& lightfield;
};
class DisparityRenderpass
{
public:
	DisparityRenderpass() = default;
	~DisparityRenderpass() = default;
	ROF_COPY_MOVE_DELETE(DisparityRenderpass)

public:
	void init(DisparityRenderpassCreateInfo& info)
	{
		create_shader_modules(info);
		//create_render_pass(info);
		create_framebuffers(info);

		//create_pipeline_layout(info);
		//create_pipeline(info);

		//create_misc(info);
	}

	void destroy(DeviceWrapper& deviceWrapper, vma::Allocator& allocator)
	{
		auto& device = deviceWrapper.logicalDevice;

		// Shaders
		device.destroyShaderModule(gradientsVS);
		device.destroyShaderModule(gradientsPS);
		device.destroyShaderModule(disparityVS);
		device.destroyShaderModule(disparityPS);

		//// Render Pass
		//deviceWrapper.logicalDevice.destroyRenderPass(renderPass);
		//for (size_t i = 0; i < framebuffers.size(); i++) {
		//	deviceWrapper.logicalDevice.destroyFramebuffer(framebuffers[i]);
		//}

		//// Stages
		//deviceWrapper.logicalDevice.destroyPipelineLayout(pipelineLayout);
		//deviceWrapper.logicalDevice.destroyPipeline(pipeline);
	}

private:
	void create_shader_modules(DisparityRenderpassCreateInfo& info)
	{
		gradientsVS = create_shader_module(info.deviceWrapper, lightfieldGradients.vs);
		gradientsPS = create_shader_module(info.deviceWrapper, lightfieldGradients.ps);

		disparityVS = create_shader_module(info.deviceWrapper, lightfieldDisparity.vs);
		disparityPS = create_shader_module(info.deviceWrapper, lightfieldDisparity.ps);
	}

	//void create_render_pass(ForwardRenderpassCreateInfo& info)
	//{
	//	std::array<vk::AttachmentDescription, 1> attachments = {
	//		// Output
	//		vk::AttachmentDescription()
	//			.setFormat(colorFormat)
	//			.setSamples(vk::SampleCountFlagBits::e1)
	//			.setLoadOp(vk::AttachmentLoadOp::eClear)
	//			.setStoreOp(vk::AttachmentStoreOp::eStore)
	//			.setStencilLoadOp(vk::AttachmentLoadOp::eDontCare)
	//			.setStencilStoreOp(vk::AttachmentStoreOp::eDontCare)
	//			.setInitialLayout(vk::ImageLayout::eUndefined)
	//			.setFinalLayout(vk::ImageLayout::eColorAttachmentOptimal)
	//	};
//
//		// Subpass Descriptions
//		vk::AttachmentReference output = vk::AttachmentReference(0, vk::ImageLayout::eColorAttachmentOptimal);
//		vk::SubpassDescription subpass = vk::SubpassDescription()
//			.setPipelineBindPoint(vk::PipelineBindPoint::eGraphics)
//			.setPDepthStencilAttachment(nullptr)
//			.setInputAttachments({}).setColorAttachments(output);
//
//		// Subpass dependency
//		vk::SubpassDependency dependency = vk::SubpassDependency()
//			.setDependencyFlags(vk::DependencyFlagBits::eByRegion)
//			// src (when/what to wait on)
//			.setSrcSubpass(VK_SUBPASS_EXTERNAL)
//			.setSrcStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput)
//			.setSrcAccessMask(vk::AccessFlagBits::eNoneKHR)
//			// dst (when/what to write to)
//			.setDstSubpass(0)
//			.setDstStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput)
//			.setDstAccessMask(vk::AccessFlagBits::eColorAttachmentWrite);
//
//		vk::RenderPassCreateInfo renderPassInfo = vk::RenderPassCreateInfo()
//			.setAttachments(attachments)
//			.setDependencies(dependency)
//			.setSubpasses(subpass);
//
//		renderPass = info.deviceWrapper.logicalDevice.createRenderPass(renderPassInfo);
//	}
	void create_framebuffers(DisparityRenderpassCreateInfo& info)
	{
		//vk::FramebufferCreateInfo framebufferInfo = vk::FramebufferCreateInfo()
		//	.setRenderPass(renderPass)
		//	.setWidth(info.swapchainWrapper.extent.width)
		//	.setHeight(info.swapchainWrapper.extent.height)
		//	.setAttachments(info.lightfield.gradientsImageView)
		//	.setLayers(1);
		//gradientsFramebuffer = info.deviceWrapper.logicalDevice.createFramebuffer(framebufferInfo);
	}
//
//	void create_pipeline_layout(ForwardRenderpassCreateInfo& info)
//	{
//		auto camLayout = Camera::get_temp_desc_set_layout(info.deviceWrapper);
//		vk::PipelineLayoutCreateInfo pipelineLayoutInfo = vk::PipelineLayoutCreateInfo()
//			.setSetLayouts(camLayout)
//			.setPushConstantRanges({});
//		pipelineLayout = info.deviceWrapper.logicalDevice.createPipelineLayout(pipelineLayoutInfo);
//		info.deviceWrapper.logicalDevice.destroyDescriptorSetLayout(camLayout);
//	}
//	void create_pipeline(ForwardRenderpassCreateInfo& info)
//	{
//		// Shaders
//		std::array<vk::PipelineShaderStageCreateInfo, 2> shaderStages;
//		{
//			shaderStages[0] = vk::PipelineShaderStageCreateInfo()
//				.setStage(vk::ShaderStageFlagBits::eVertex)
//				.setModule(vs)
//				.setPName("main")
//				.setPSpecializationInfo(nullptr); // constants for optimization
//
//			shaderStages[1] = vk::PipelineShaderStageCreateInfo()
//				.setStage(vk::ShaderStageFlagBits::eFragment)
//				.setModule(ps)
//				.setPName("main")
//				.setPSpecializationInfo(nullptr);
//		}
//
//		// Input
//		vk::PipelineVertexInputStateCreateInfo vertexInputInfo;
//		vk::PipelineInputAssemblyStateCreateInfo inputAssemplyInfo;
//		auto attrDesc = Vertex::get_attr_desc();
//		auto bindingDesc = Vertex::get_binding_desc();
//		{
//			// Vertex Input descriptor
//			vertexInputInfo = vk::PipelineVertexInputStateCreateInfo()
//				.setVertexBindingDescriptionCount(1).setVertexBindingDescriptions(bindingDesc)
//				.setVertexAttributeDescriptionCount(attrDesc.size()).setVertexAttributeDescriptions(attrDesc);
//
//			// Input Assembly
//			inputAssemplyInfo = vk::PipelineInputAssemblyStateCreateInfo()
//				.setTopology(vk::PrimitiveTopology::eTriangleList)
//				.setPrimitiveRestartEnable(VK_FALSE);
//		}
//
//		// Viewport
//		vk::Rect2D scissorRect;
//		vk::Viewport viewport;
//		vk::PipelineViewportStateCreateInfo viewportStateInfo;
//		{
//			// Scissor Rect
//			scissorRect = vk::Rect2D()
//				.setOffset({ 0, 0 })
//				.setExtent(info.swapchainWrapper.extent);
//			// Viewport
//			viewport = vk::Viewport()
//				.setX(0.0f).setY(0.0f)
//				.setMinDepth(0.0f).setMaxDepth(1.0f)
//				.setWidth(static_cast<float>(info.swapchainWrapper.extent.width))
//				.setHeight(static_cast<float>(info.swapchainWrapper.extent.height));
//
//			// Viewport state creation
//			viewportStateInfo = vk::PipelineViewportStateCreateInfo()
//				.setViewportCount(1).setPViewports(&viewport)
//				.setScissorCount(1).setPScissors(&scissorRect);
//		}
//
//		// Rasterization and Multisampling
//		vk::PipelineRasterizationStateCreateInfo rasterizerInfo;
//		vk::PipelineMultisampleStateCreateInfo multisamplingInfo;
//		{
//			// Rasterizer
//			rasterizerInfo = vk::PipelineRasterizationStateCreateInfo()
//				.setDepthClampEnable(VK_FALSE)
//				.setRasterizerDiscardEnable(VK_FALSE)
//				.setPolygonMode(vk::PolygonMode::eFill)
//				.setLineWidth(1.0f)
//				.setCullMode(vk::CullModeFlagBits::eBack)
//				.setFrontFace(vk::FrontFace::eClockwise)
//				.setDepthBiasEnable(VK_FALSE)
//				.setDepthBiasConstantFactor(0.0f)
//				.setDepthBiasClamp(0.0f)
//				.setDepthBiasSlopeFactor(0.0f);
//
//			// Multisampling
//			multisamplingInfo = vk::PipelineMultisampleStateCreateInfo()
//				.setSampleShadingEnable(VK_FALSE)
//				.setRasterizationSamples(vk::SampleCountFlagBits::e1)
//				.setMinSampleShading(1.0f)
//				.setPSampleMask(nullptr)
//				.setAlphaToCoverageEnable(VK_FALSE)
//				.setAlphaToOneEnable(VK_FALSE);
//		}
//
//		// Color Blending
//		vk::PipelineColorBlendAttachmentState colorBlendAttachment;
//		vk::PipelineColorBlendStateCreateInfo colorBlendInfo;
//		{
//			// final output image
//			colorBlendAttachment = vk::PipelineColorBlendAttachmentState()
//				.setColorWriteMask(
//					vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
//					vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA) // theoretically no need to write transparency
//				.setBlendEnable(VK_FALSE)
//				// color blend
//				.setSrcColorBlendFactor(vk::BlendFactor::eOne)
//				.setDstColorBlendFactor(vk::BlendFactor::eZero)
//				.setColorBlendOp(vk::BlendOp::eAdd)
//				// alpha blend
//				.setSrcAlphaBlendFactor(vk::BlendFactor::eOne)
//				.setDstAlphaBlendFactor(vk::BlendFactor::eZero)
//				.setAlphaBlendOp(vk::BlendOp::eAdd);
//
//			// -> global
//			colorBlendInfo = vk::PipelineColorBlendStateCreateInfo()
//				.setLogicOpEnable(VK_FALSE).setLogicOp(vk::LogicOp::eCopy)
//				.setAttachments(colorBlendAttachment)
//				.setBlendConstants({ 0.0f, 0.0f, 0.0f, 0.0f });
//		}
//
//		// Depth Stencil
//		vk::PipelineDepthStencilStateCreateInfo depthStencilInfo;
//		{
//			depthStencilInfo = vk::PipelineDepthStencilStateCreateInfo()
//				.setDepthTestEnable(VK_FALSE)
//				.setDepthWriteEnable(VK_FALSE)
//				.setDepthCompareOp(vk::CompareOp::eLessOrEqual)
//				// Depth bounds
//				.setDepthBoundsTestEnable(VK_FALSE)
//				.setMinDepthBounds(0.0f).setMaxDepthBounds(1.0f)
//				// Stencil
//				.setStencilTestEnable(VK_FALSE);
//		}
//
//		// Finally, create actual render pipeline
//		vk::GraphicsPipelineCreateInfo graphicsPipelineInfo = vk::GraphicsPipelineCreateInfo()
//			.setStageCount(shaderStages.size())
//			.setPStages(shaderStages.data())
//			// fixed-function stages
//			.setPVertexInputState(&vertexInputInfo)
//			.setPInputAssemblyState(&inputAssemplyInfo)
//			.setPViewportState(&viewportStateInfo)
//			.setPRasterizationState(&rasterizerInfo)
//			.setPMultisampleState(&multisamplingInfo)
//			.setPDepthStencilState(&depthStencilInfo)
//			.setPColorBlendState(&colorBlendInfo)
//			.setPDynamicState(nullptr)
//			// pipeline layout
//			.setLayout(pipelineLayout)
//			// render pass
//			.setRenderPass(renderPass)
//			.setSubpass(0);
//
//		auto result = info.deviceWrapper.logicalDevice.createGraphicsPipeline(pipelineCache, graphicsPipelineInfo);
//		switch (result.result)
//		{
//			case vk::Result::eSuccess: break;
//			case vk::Result::ePipelineCompileRequiredEXT:
//				VMI_LOG("Graphics pipeline creation: PipelineCompileRequiredEXT");
//				break;
//			default: assert(false);
//		}
//		pipeline = result.value;
//	}
//
//	void create_misc(ForwardRenderpassCreateInfo& info)
//	{
//		fullscreenRect = vk::Rect2D({ 0, 0 }, info.swapchainWrapper.extent);
//		clearValues = {
//			vk::ClearValue(vk::ClearColorValue().setFloat32({ 0.0f, 0.0f, 0.0f, 0.0f }))
//		};
//	}

private:
	static constexpr vk::Format colorFormat = vk::Format::eR8G8B8A8Srgb;
	static constexpr uint32_t nCams = 9;
	vk::RenderPass renderPass;

	// subpasses
	vk::Pipeline gradientsPipeline, disparityPipeline;
	vk::PipelineLayout gradientsPipelineLayout, disparityPipelineLayout;
	vk::PipelineCache gradientsPipelineCache, disparityPipelineCache; // TODO

	// shaders for the subpasses
	vk::ShaderModule gradientsVS, gradientsPS;
	vk::ShaderModule disparityVS, disparityPS;

	// render resources
	vk::Framebuffer gradientsFramebuffer;
	vk::Framebuffer disparityFramebuffer;

	// misc
	vk::Rect2D fullscreenRect;
};