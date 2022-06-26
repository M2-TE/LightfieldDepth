#pragma once

struct DeferredRenderpassCreateInfo
{
	DeviceWrapper& deviceWrapper;
	SwapchainWrapper& swapchainWrapper;
	vma::Allocator& allocator;
	vk::DescriptorPool& descPool;
};

#include "gbuffer.hpp"

class DeferredRenderpass
{
public:
	DeferredRenderpass() = default;
	~DeferredRenderpass() = default;
	ROF_COPY_MOVE_DELETE(DeferredRenderpass)

public:
	void init(DeferredRenderpassCreateInfo& info)
	{
		// Shaders
		create_shader_modules(info);

		// Input/Output
		gbuffer.init(info);

		// Render Pass
		create_render_pass(info);
		create_framebuffer(info);
		// Stages
		create_geometry_pass_pipeline_layout(info);
		create_geometry_pass_pipeline(info);
		create_lighting_pass_pipeline_layout(info);
		create_lighting_pass_pipeline(info);

		// other
		create_misc(info);
	}
	void destroy(DeviceWrapper& deviceWrapper, vma::Allocator& allocator)
	{
		auto& device = deviceWrapper.logicalDevice;

		// Shaders
		device.destroyShaderModule(geometryVS);
		device.destroyShaderModule(geometryPS);
		device.destroyShaderModule(lightingVS);
		device.destroyShaderModule(lightingPS);

		// Input/Output
		gbuffer.destroy(deviceWrapper, allocator);

		// Render Pass
		deviceWrapper.logicalDevice.destroyRenderPass(renderPass);
		deviceWrapper.logicalDevice.destroyFramebuffer(framebuffer);

		// Stages
		deviceWrapper.logicalDevice.destroyPipelineLayout(geometryPassPipelineLayout);
		deviceWrapper.logicalDevice.destroyPipeline(geometryPassPipeline);
		deviceWrapper.logicalDevice.destroyPipelineLayout(lightingPassPipelineLayout);
		deviceWrapper.logicalDevice.destroyPipeline(lightingPassPipeline);
	}

	void begin(vk::CommandBuffer& commandBuffer)
	{
		vk::RenderPassBeginInfo renderPassBeginInfo = vk::RenderPassBeginInfo()
			.setRenderPass(renderPass)
			.setFramebuffer(framebuffer)
			.setRenderArea(fullscreenRect)
			.setClearValues(clearValues);

		commandBuffer.beginRenderPass(renderPassBeginInfo, vk::SubpassContents::eInline);
		commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, geometryPassPipeline);
	}
	void end(vk::CommandBuffer& commandBuffer)
	{
		// draw fullscreen triangle to write output
		commandBuffer.nextSubpass(vk::SubpassContents::eInline);
		commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, lightingPassPipeline);

		commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, lightingPassPipelineLayout, 0, gbuffer.descSet, {});
		commandBuffer.draw(3, 1, 0, 0);

		// write imgui ui to the output image
		ImGui::Render();
		ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), commandBuffer);

		commandBuffer.endRenderPass();
	}
	void bind_desc_sets(vk::CommandBuffer& commandBuffer, const vk::ArrayProxy<vk::DescriptorSet>& descSets)
	{
		commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, geometryPassPipelineLayout, 0, descSets, {});
	}

	// TODO: remove some of these
	inline vk::RenderPass& get_render_pass() { return renderPass; }
	inline vk::Pipeline& get_geometry_pass() { return geometryPassPipeline; }
	inline vk::Pipeline& get_lighting_pass() { return lightingPassPipeline; }
	inline vk::PipelineLayout& get_geometry_pass_layout() { return geometryPassPipelineLayout; }
	inline vk::PipelineLayout& get_lighting_pass_layout() { return lightingPassPipelineLayout; }
	inline vk::DescriptorSet& get_descriptor_set() { return gbuffer.descSet; }
	inline vk::Image& get_output_image() { return gbuffer.outputImage; }
	inline vk::ImageView& get_output_image_view() { return gbuffer.outputImageView; }
	inline vk::Framebuffer& get_framebuffer() { return framebuffer; }

private:
	void create_shader_modules(DeferredRenderpassCreateInfo& info)
	{
		geometryVS = create_shader_module(info.deviceWrapper, geometryPass.vs);
		geometryPS = create_shader_module(info.deviceWrapper, geometryPass.ps);

		lightingVS = create_shader_module(info.deviceWrapper, lightingPass.vs);
		lightingPS = create_shader_module(info.deviceWrapper, lightingPass.ps);
	}
	void create_framebuffer(DeferredRenderpassCreateInfo& info)
	{
		std::array<vk::ImageView, 5> attachments = {
			gbuffer.posImageView,
			gbuffer.colImageView,
			gbuffer.normImageView,
			gbuffer.outputImageView,
			gbuffer.depthStencilView
		};

		vk::FramebufferCreateInfo framebufferInfo = vk::FramebufferCreateInfo()
			.setRenderPass(renderPass)
			.setWidth(info.swapchainWrapper.extent.width)
			.setHeight(info.swapchainWrapper.extent.height)
			.setAttachments(attachments)
			.setLayers(1);

		framebuffer = info.deviceWrapper.logicalDevice.createFramebuffer(framebufferInfo);
	}
	void create_misc(DeferredRenderpassCreateInfo& info)
	{
		fullscreenRect = vk::Rect2D({ 0, 0 }, info.swapchainWrapper.extent);
		clearValues = {
			vk::ClearValue(vk::ClearColorValue().setFloat32({ 0.0f, 0.0f, 0.0f, 0.0f })),
			vk::ClearValue(vk::ClearColorValue().setFloat32({ 0.0f, 0.0f, 0.0f, 0.0f })),
			vk::ClearValue(vk::ClearColorValue().setFloat32({ 0.0f, 0.0f, 0.0f, 0.0f })),
			vk::ClearValue(vk::ClearColorValue().setFloat32({ 0.0f, 0.0f, 0.0f, 0.0f })),
			vk::ClearValue(vk::ClearDepthStencilValue().setDepth(1.0f).setStencil(0))
		};
	}

	// render pass creation
	void fill_attachment_descriptions(DeferredRenderpassCreateInfo& info, std::array<vk::AttachmentDescription, GBuffer::nImages + 2>& attachments)
	{
		// gbuffer pos attachment
		{
			attachments[0] = vk::AttachmentDescription()
				.setFormat(vk::Format::eR32G32B32A32Sfloat)
				.setSamples(vk::SampleCountFlagBits::e1)
				.setLoadOp(vk::AttachmentLoadOp::eClear)
				.setStoreOp(vk::AttachmentStoreOp::eStore)
				.setStencilLoadOp(vk::AttachmentLoadOp::eDontCare)
				.setStencilStoreOp(vk::AttachmentStoreOp::eDontCare)
				.setInitialLayout(vk::ImageLayout::eUndefined)
				.setFinalLayout(vk::ImageLayout::eShaderReadOnlyOptimal);
		}
		// gbuffer col attachment
		{
			attachments[1] = vk::AttachmentDescription()
				.setFormat(vk::Format::eR8G8B8A8Srgb)
				.setSamples(vk::SampleCountFlagBits::e1)
				.setLoadOp(vk::AttachmentLoadOp::eClear)
				.setStoreOp(vk::AttachmentStoreOp::eStore)
				.setStencilLoadOp(vk::AttachmentLoadOp::eDontCare)
				.setStencilStoreOp(vk::AttachmentStoreOp::eDontCare)
				.setInitialLayout(vk::ImageLayout::eUndefined)
				.setFinalLayout(vk::ImageLayout::eShaderReadOnlyOptimal);
		}
		// gbuffer norm attachment
		{
			attachments[2] = vk::AttachmentDescription()
				.setFormat(vk::Format::eR8G8B8A8Snorm)
				.setSamples(vk::SampleCountFlagBits::e1)
				.setLoadOp(vk::AttachmentLoadOp::eClear)
				.setStoreOp(vk::AttachmentStoreOp::eStore)
				.setStencilLoadOp(vk::AttachmentLoadOp::eDontCare)
				.setStencilStoreOp(vk::AttachmentStoreOp::eDontCare)
				.setInitialLayout(vk::ImageLayout::eUndefined)
				.setFinalLayout(vk::ImageLayout::eShaderReadOnlyOptimal);
		}
		// output color attachment
		{
			attachments[3] = vk::AttachmentDescription()
				.setFormat(gbuffer.colorFormat)
				.setSamples(vk::SampleCountFlagBits::e1)
				.setLoadOp(vk::AttachmentLoadOp::eClear)
				.setStoreOp(vk::AttachmentStoreOp::eStore)
				.setStencilLoadOp(vk::AttachmentLoadOp::eDontCare)
				.setStencilStoreOp(vk::AttachmentStoreOp::eDontCare)
				.setInitialLayout(vk::ImageLayout::eUndefined)
				.setFinalLayout(vk::ImageLayout::eColorAttachmentOptimal);
		}
		// depth stencil attachment
		{
			attachments[4] = vk::AttachmentDescription()
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
		}
	}
	void fill_subpass_dependencies(std::array<vk::SubpassDependency, 2>& dependencies)
	{
		// geometry pass
		{
			dependencies[0] = vk::SubpassDependency()
				.setDependencyFlags(vk::DependencyFlagBits::eByRegion) // for tiled GPUs.. i think.
				// src (when/what to wait on)
				.setSrcSubpass(VK_SUBPASS_EXTERNAL)
				.setSrcStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput)
				.setSrcAccessMask(vk::AccessFlagBits::eNoneKHR)
				// dst (when/what to write to)
				.setDstSubpass(0)
				.setDstStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput | vk::PipelineStageFlagBits::eEarlyFragmentTests)
				.setDstAccessMask(vk::AccessFlagBits::eColorAttachmentWrite | vk::AccessFlagBits::eDepthStencilAttachmentWrite);
		}

		// lighting pass
		{
			dependencies[1] = vk::SubpassDependency()
				.setDependencyFlags(vk::DependencyFlagBits::eByRegion) // for tiled GPUs.. i think.
				// src (when/what to wait on)
				.setSrcSubpass(0)
				.setSrcStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput | vk::PipelineStageFlagBits::eEarlyFragmentTests)
				.setSrcAccessMask(vk::AccessFlagBits::eColorAttachmentRead)
				// dst (when/what to write to)
				.setDstSubpass(1)
				.setDstStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput)
				.setDstAccessMask(vk::AccessFlagBits::eColorAttachmentWrite);
		}
	}
	void create_render_pass(DeferredRenderpassCreateInfo& info)
	{
		std::array<vk::AttachmentDescription, GBuffer::nImages + 2> attachments;
		fill_attachment_descriptions(info, attachments);

		// Subpass Descriptions
		std::array<vk::SubpassDescription, 2> subpasses;
		vk::AttachmentReference depthAttachmentRef;
		std::vector<vk::AttachmentReference> inputGeometry, outputGeometry;
		std::vector<vk::AttachmentReference> inputLighting, outputLighting;
		{
			// Geometry Pass
			{
				depthAttachmentRef = vk::AttachmentReference(4, vk::ImageLayout::eDepthStencilAttachmentOptimal);

				inputGeometry = {
				};

				outputGeometry = {
					vk::AttachmentReference(0, vk::ImageLayout::eColorAttachmentOptimal), // gPos
					vk::AttachmentReference(1, vk::ImageLayout::eColorAttachmentOptimal), // gCol
					vk::AttachmentReference(2, vk::ImageLayout::eColorAttachmentOptimal), // gNorm
				};

				subpasses[0] = vk::SubpassDescription()
					.setPipelineBindPoint(vk::PipelineBindPoint::eGraphics)
					.setPDepthStencilAttachment(&depthAttachmentRef)
					.setInputAttachments(inputGeometry)
					.setColorAttachments(outputGeometry)
					// misc other:
					.setPreserveAttachmentCount(0).setPPreserveAttachments(nullptr)
					.setPResolveAttachments(nullptr);
			}

			// Lighting Pass
			{
				inputLighting = {
					vk::AttachmentReference(0, vk::ImageLayout::eShaderReadOnlyOptimal), // gPos
					vk::AttachmentReference(1, vk::ImageLayout::eShaderReadOnlyOptimal), // gCol
					vk::AttachmentReference(2, vk::ImageLayout::eShaderReadOnlyOptimal), // gNorm
				};

				outputLighting = {
					vk::AttachmentReference(3, vk::ImageLayout::eColorAttachmentOptimal), // output image
				};

				subpasses[1] = vk::SubpassDescription()
					.setPipelineBindPoint(vk::PipelineBindPoint::eGraphics)
					.setPDepthStencilAttachment(nullptr)
					.setInputAttachments(inputLighting)
					.setColorAttachments(outputLighting)
					// misc other:
					.setPreserveAttachmentCount(0).setPPreserveAttachments(nullptr)
					.setPResolveAttachments(nullptr);
			}
		}

		std::array<vk::SubpassDependency, 2> dependencies;
		fill_subpass_dependencies(dependencies);

		vk::RenderPassCreateInfo renderPassInfo = vk::RenderPassCreateInfo()
			.setAttachments(attachments)
			.setDependencies(dependencies)
			.setSubpasses(subpasses);

		renderPass = info.deviceWrapper.logicalDevice.createRenderPass(renderPassInfo);
	}

	// graphics pipelines
	void create_geometry_pass_pipeline_layout(DeferredRenderpassCreateInfo& info)
	{
		auto camLayout = Camera::get_temp_desc_set_layout(info.deviceWrapper);
		vk::PipelineLayoutCreateInfo pipelineLayoutInfo = vk::PipelineLayoutCreateInfo()
			.setSetLayouts(camLayout)
			.setPushConstantRangeCount(0).setPushConstantRanges(nullptr);
		geometryPassPipelineLayout = info.deviceWrapper.logicalDevice.createPipelineLayout(pipelineLayoutInfo);
		info.deviceWrapper.logicalDevice.destroyDescriptorSetLayout(camLayout);
	}
	void create_geometry_pass_pipeline(DeferredRenderpassCreateInfo& info)
	{
		// Shaders
		std::array<vk::PipelineShaderStageCreateInfo, 2> shaderStages;
		{
			shaderStages[0] = vk::PipelineShaderStageCreateInfo()
				.setStage(vk::ShaderStageFlagBits::eVertex)
				.setModule(geometryVS)
				// entrypoint (one shader module with multiple entry points for different shading stages?)
				.setPName("main")
				.setPSpecializationInfo(nullptr); // constants for optimization

			shaderStages[1] = vk::PipelineShaderStageCreateInfo()
				.setStage(vk::ShaderStageFlagBits::eFragment)
				.setModule(geometryPS)
				.setPName("main")
				.setPSpecializationInfo(nullptr);
		}

		// Input
		vk::PipelineVertexInputStateCreateInfo vertexInputInfo;
		vk::PipelineInputAssemblyStateCreateInfo inputAssemplyInfo;
		auto attrDesc = Vertex::get_attr_desc();
		auto bindingDesc = Vertex::get_binding_desc();
		{
			// Vertex Input descriptor
			vertexInputInfo = vk::PipelineVertexInputStateCreateInfo()
				.setVertexBindingDescriptionCount(1).setVertexBindingDescriptions(bindingDesc)
				.setVertexAttributeDescriptionCount(attrDesc.size()).setVertexAttributeDescriptions(attrDesc);

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
				.setX(0.0f)
				.setY(0.0f)
				.setWidth(static_cast<float>(info.swapchainWrapper.extent.width))
				.setHeight(static_cast<float>(info.swapchainWrapper.extent.height))
				.setMinDepth(0.0f)
				.setMaxDepth(1.0f);

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
		std::array<vk::PipelineColorBlendAttachmentState, GBuffer::nImages> colorBlendAttachments;
		vk::PipelineColorBlendStateCreateInfo colorBlendInfo;
		{
			for (size_t i = 0; i < colorBlendAttachments.size(); i++) {

				// GBuffer output image
				colorBlendAttachments[i] = vk::PipelineColorBlendAttachmentState()
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
			}
			
			// -> global
			colorBlendInfo = vk::PipelineColorBlendStateCreateInfo()
				.setLogicOpEnable(VK_FALSE).setLogicOp(vk::LogicOp::eCopy)
				.setAttachments(colorBlendAttachments)
				.setBlendConstants({ 0.0f, 0.0f, 0.0f, 0.0f });
		}

		// Depth Stencil
		vk::PipelineDepthStencilStateCreateInfo depthStencilInfo;
		{
			depthStencilInfo = vk::PipelineDepthStencilStateCreateInfo()
				.setDepthTestEnable(VK_TRUE)
				.setDepthWriteEnable(VK_TRUE)
				.setDepthCompareOp(vk::CompareOp::eLessOrEqual)
				// Depth bounds
				.setDepthBoundsTestEnable(VK_FALSE)
				.setMinDepthBounds(0.0f).setMaxDepthBounds(1.0f)
				// Stencil
				.setStencilTestEnable(VK_FALSE);
		}

		// Finally, create actual render pipeline
		vk::GraphicsPipelineCreateInfo graphicsPipelineInfo = vk::GraphicsPipelineCreateInfo()
			.setStageCount(shaderStages.size())
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
			.setLayout(geometryPassPipelineLayout)
			// render pass
			.setRenderPass(renderPass)
			.setSubpass(0);

		auto result = info.deviceWrapper.logicalDevice.createGraphicsPipeline(geometryPassPipelineCache, graphicsPipelineInfo);
		switch (result.result)
		{
			case vk::Result::eSuccess: break;
			case vk::Result::ePipelineCompileRequiredEXT:
				VMI_LOG("Graphics pipeline creation: PipelineCompileRequiredEXT");
				break;
			default: assert(false);
		}
		geometryPassPipeline = result.value;
	}
	void create_lighting_pass_pipeline_layout(DeferredRenderpassCreateInfo& info)
	{
		vk::PipelineLayoutCreateInfo pipelineLayoutInfo = vk::PipelineLayoutCreateInfo()
			.setSetLayouts(gbuffer.descSetLayout)
			.setPushConstantRangeCount(0).setPushConstantRanges(nullptr);
		lightingPassPipelineLayout = info.deviceWrapper.logicalDevice.createPipelineLayout(pipelineLayoutInfo);
	}
	void create_lighting_pass_pipeline(DeferredRenderpassCreateInfo& info)
	{
		// Shaders
		std::array<vk::PipelineShaderStageCreateInfo, 2> shaderStages;
		{
			shaderStages[0] = vk::PipelineShaderStageCreateInfo()
				.setStage(vk::ShaderStageFlagBits::eVertex)
				.setModule(lightingVS)
				// entrypoint (one shader module with multiple entry points for different shading stages?)
				.setPName("main")
				.setPSpecializationInfo(nullptr); // constants for optimization

			shaderStages[1] = vk::PipelineShaderStageCreateInfo()
				.setStage(vk::ShaderStageFlagBits::eFragment)
				.setModule(lightingPS)
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
		std::array<vk::PipelineColorBlendAttachmentState, 1> colorBlendAttachments;
		vk::PipelineColorBlendStateCreateInfo colorBlendInfo;
		{
			for (size_t i = 0; i < colorBlendAttachments.size(); i++) {

				// final output image
				colorBlendAttachments[i] = vk::PipelineColorBlendAttachmentState()
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
			}

			// -> global
			colorBlendInfo = vk::PipelineColorBlendStateCreateInfo()
				.setLogicOpEnable(VK_FALSE).setLogicOp(vk::LogicOp::eCopy)
				.setAttachments(colorBlendAttachments)
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
			.setStageCount(shaderStages.size())
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
			.setLayout(lightingPassPipelineLayout)
			// render pass
			.setRenderPass(renderPass)
			.setSubpass(1);

		auto result = info.deviceWrapper.logicalDevice.createGraphicsPipeline(lightingPassPipelineCache, graphicsPipelineInfo);
		switch (result.result)
		{
			case vk::Result::eSuccess: break;
			case vk::Result::ePipelineCompileRequiredEXT:
				VMI_LOG("Graphics pipeline creation: PipelineCompileRequiredEXT");
				break;
			default: assert(false);
		}
		lightingPassPipeline = result.value;
	}

private:
	vk::RenderPass renderPass;

	// subpasses
	vk::Pipeline geometryPassPipeline, lightingPassPipeline;
	vk::PipelineLayout geometryPassPipelineLayout, lightingPassPipelineLayout;
	vk::PipelineCache geometryPassPipelineCache, lightingPassPipelineCache; // TODO

	// shaders for the subpasses
	vk::ShaderModule geometryVS, geometryPS;
	vk::ShaderModule lightingVS, lightingPS;

	// render resources
	GBuffer gbuffer;
	vk::Framebuffer framebuffer;

	// misc
	vk::Rect2D fullscreenRect;
	std::array<vk::ClearValue, 5> clearValues;
};