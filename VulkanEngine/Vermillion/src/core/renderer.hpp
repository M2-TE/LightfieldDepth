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
		// track sync frames separately, since we need a semaphore even before acquiring an image index
		SyncFrame& syncFrame = ringBuffer.advance().get_sync_frame();

		uint32_t iFrame;
		// Acquire image
		{
			vk::ResultValue imgResult = deviceWrapper.logicalDevice.acquireNextImageKHR(swapchainWrapper.swapchain, UINT64_MAX, syncFrame.imageAvailable);
			switch (imgResult.result) {
				case vk::Result::eSuccess: break;
				case vk::Result::eSuboptimalKHR: VMI_LOG("Suboptimal image acquisition."); break;
				case vk::Result::eErrorOutOfDateKHR: VMI_ERR("Swapchain: KHR out of date."); assert(false);
				default: assert(false);
			}
			iFrame = imgResult.value;
		}
		RingFrame& frame = ringBuffer.frames[iFrame];

		// Render
		{
			// wait for fence of fetched frame before rendering to it
			vk::Result result = deviceWrapper.logicalDevice.waitForFences(syncFrame.fence, VK_TRUE, UINT64_MAX);
			if (result != vk::Result::eSuccess) assert(false);

			// reset command pool and then record into it (using command buffer)
			deviceWrapper.logicalDevice.resetCommandPool(frame.commandPool);
			update_uniform_buffer(deviceWrapper, iFrame);
			record_command_buffer(iFrame);
		}

		// Present
		{
			vk::PipelineStageFlags waitStages = vk::PipelineStageFlagBits::eColorAttachmentOutput;
			vk::SubmitInfo submitInfo = vk::SubmitInfo()
				.setPWaitDstStageMask(&waitStages)
				// semaphores
				.setWaitSemaphoreCount(1).setPWaitSemaphores(&syncFrame.imageAvailable)
				.setSignalSemaphoreCount(1).setPSignalSemaphores(&syncFrame.renderFinished)
				// command buffers
				.setCommandBufferCount(1).setPCommandBuffers(&frame.commandBuffer);

			deviceWrapper.logicalDevice.resetFences(syncFrame.fence);
			deviceWrapper.queue.submit(submitInfo, syncFrame.fence);

			vk::PresentInfoKHR presentInfo = vk::PresentInfoKHR()
				.setPImageIndices(&iFrame)
				// semaphores
				.setWaitSemaphoreCount(1).setPWaitSemaphores(&syncFrame.renderFinished)
				// swapchains
				.setSwapchainCount(1).setPSwapchains(&swapchainWrapper.swapchain);
			vk::Result result = deviceWrapper.queue.presentKHR(&presentInfo);
			if (result != vk::Result::eSuccess) assert(false);
		}
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

		ringBuffer.create_all(deviceWrapper, allocator, renderPass, swapchainWrapper, descPool);
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
		std::array<vk::AttachmentDescription, 5> attachments;
		{
			// color attachment
			{
				attachments[0] = vk::AttachmentDescription()
					.setFormat(swapchainWrapper.surfaceFormat.format)
					.setSamples(vk::SampleCountFlagBits::e1)
					.setLoadOp(vk::AttachmentLoadOp::eClear)
					.setStoreOp(vk::AttachmentStoreOp::eStore)
					.setStencilLoadOp(vk::AttachmentLoadOp::eDontCare)
					.setStencilStoreOp(vk::AttachmentStoreOp::eDontCare)
					.setInitialLayout(vk::ImageLayout::eUndefined)
					.setFinalLayout(vk::ImageLayout::ePresentSrcKHR);
			}
			// depth stencil attachment
			{
				attachments[1] = vk::AttachmentDescription()
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
			// gbuffer pos attachment
			{
				attachments[2] = vk::AttachmentDescription()
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
				attachments[3] = vk::AttachmentDescription()
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
				attachments[4] = vk::AttachmentDescription()
					.setFormat(vk::Format::eR8G8B8A8Snorm)
					.setSamples(vk::SampleCountFlagBits::e1)
					.setLoadOp(vk::AttachmentLoadOp::eClear)
					.setStoreOp(vk::AttachmentStoreOp::eStore)
					.setStencilLoadOp(vk::AttachmentLoadOp::eDontCare)
					.setStencilStoreOp(vk::AttachmentStoreOp::eDontCare)
					.setInitialLayout(vk::ImageLayout::eUndefined)
					.setFinalLayout(vk::ImageLayout::eShaderReadOnlyOptimal);
			}
		}
		
		std::array<vk::SubpassDescription, 1> subpasses;
		{
			// first subpass (write to gbuffer and depth)
			{
				vk::AttachmentReference colorAttachmentRef = vk::AttachmentReference(0, vk::ImageLayout::eColorAttachmentOptimal);
				vk::AttachmentReference depthAttachmentRef = vk::AttachmentReference(1, vk::ImageLayout::eDepthStencilAttachmentOptimal);

				subpasses[0] = vk::SubpassDescription()
					.setPipelineBindPoint(vk::PipelineBindPoint::eGraphics)
					.setPDepthStencilAttachment(&depthAttachmentRef)
					.setColorAttachmentCount(1).setPColorAttachments(&colorAttachmentRef)
					.setInputAttachmentCount(0).setPInputAttachments(nullptr)
					// misc other:
					.setPreserveAttachmentCount(0).setPPreserveAttachments(nullptr)
					.setPResolveAttachments(nullptr);
			}

			// second subpass (read gbuffer, write to swapchain)
			if(false)
			{
				std::array<vk::AttachmentReference, 4> input = {
					vk::AttachmentReference(1, vk::ImageLayout::eShaderReadOnlyOptimal), // depth
					vk::AttachmentReference(2, vk::ImageLayout::eShaderReadOnlyOptimal), // gPos
					vk::AttachmentReference(3, vk::ImageLayout::eShaderReadOnlyOptimal), // gCol
					vk::AttachmentReference(4, vk::ImageLayout::eShaderReadOnlyOptimal), // gNorm
				};
				vk::AttachmentReference swapchainRef = vk::AttachmentReference(0, vk::ImageLayout::eColorAttachmentOptimal);

				subpasses[1] = vk::SubpassDescription()
					.setPipelineBindPoint(vk::PipelineBindPoint::eGraphics)
					.setPDepthStencilAttachment(nullptr)
					.setColorAttachmentCount(1).setPColorAttachments(&swapchainRef)
					.setInputAttachmentCount(input.size()).setPInputAttachments(input.data())
					// misc other:
					.setPreserveAttachmentCount(0).setPPreserveAttachments(nullptr)
					.setPResolveAttachments(nullptr);
			}
		}

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

		vk::RenderPassCreateInfo renderPassInfo = vk::RenderPassCreateInfo()
			.setAttachmentCount(attachments.size()).setPAttachments(attachments.data())
			.setSubpassCount(subpasses.size()).setPSubpasses(subpasses.data())
			.setDependencyCount(1).setPDependencies(&dependency);

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
		
		// -> for each color attachment
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
			.setLogicOpEnable(VK_FALSE).setLogicOp(vk::LogicOp::eCopy)
			.setAttachmentCount(1).setPAttachments(&colorBlendAttachment)
			.setBlendConstants({ 0.0f, 0.0f, 0.0f, 0.0f });

		vk::PipelineDepthStencilStateCreateInfo depthStencilInfo = vk::PipelineDepthStencilStateCreateInfo()
			.setDepthTestEnable(VK_TRUE)
			.setDepthWriteEnable(VK_TRUE)
			.setDepthCompareOp(vk::CompareOp::eLessOrEqual)
			// Depth bounds
			.setDepthBoundsTestEnable(VK_FALSE)
			.setMinDepthBounds(0.0f).setMaxDepthBounds(1.0f)
			// Stencil
			.setStencilTestEnable(VK_FALSE);

		// Pipeline Layout
		vk::PipelineLayoutCreateInfo pipelineLayoutInfo = vk::PipelineLayoutCreateInfo()
			.setSetLayoutCount(1).setSetLayouts(mvpBuffer.get_desc_set_layout())
			.setPushConstantRangeCount(0).setPushConstantRanges(nullptr);
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
			.setSubpass(0); // index for the subpass this render pipeline will use

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

	void create_geometry_pass_graphics_pipeline(DeviceWrapper& deviceWrapper)
	{
		vk::PipelineShaderStageCreateInfo vertStageInfo = vk::PipelineShaderStageCreateInfo()
			.setStage(vk::ShaderStageFlagBits::eVertex)
			.setModule(geometry_vs)
			// entrypoint (one shader module with multiple entry points for different shading stages?)
			.setPName("main")
			.setPSpecializationInfo(nullptr); // constants for optimization

		vk::PipelineShaderStageCreateInfo fragStageInfo = vk::PipelineShaderStageCreateInfo()
			.setStage(vk::ShaderStageFlagBits::eFragment) // fragment == pixel shader in hlsl
			.setModule(geometry_ps)
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
		// 
		// -> for each color attachment
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
		std::array<vk::PipelineColorBlendAttachmentState, 3> gbufferBlendAttachments = {
			colorBlendAttachment, colorBlendAttachment, colorBlendAttachment
		};
		//
		// -> global
		vk::PipelineColorBlendStateCreateInfo colorBlendInfo = vk::PipelineColorBlendStateCreateInfo()
			.setLogicOpEnable(VK_FALSE).setLogicOp(vk::LogicOp::eCopy)
			.setAttachmentCount(gbufferBlendAttachments.size()).setPAttachments(gbufferBlendAttachments.data())
			.setBlendConstants({ 0.0f, 0.0f, 0.0f, 0.0f });

		vk::PipelineDepthStencilStateCreateInfo depthStencilInfo = vk::PipelineDepthStencilStateCreateInfo()
			.setDepthTestEnable(VK_TRUE)
			.setDepthWriteEnable(VK_TRUE)
			.setDepthCompareOp(vk::CompareOp::eLessOrEqual)
			// Depth bounds
			.setDepthBoundsTestEnable(VK_FALSE)
			.setMinDepthBounds(0.0f).setMaxDepthBounds(1.0f)
			// Stencil
			.setStencilTestEnable(VK_FALSE);

		// Pipeline Layout
		vk::PipelineLayoutCreateInfo pipelineLayoutInfo = vk::PipelineLayoutCreateInfo()
			.setSetLayoutCount(1).setPSetLayouts(&mvpBuffer.get_desc_set_layout())
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
			.setSubpass(0);

		auto result = deviceWrapper.logicalDevice.createGraphicsPipeline(nullptr, graphicsPipelineInfo);
		geometryPassPipeline = result.value;
		switch (result.result)
		{
			case vk::Result::eSuccess: break;
			case vk::Result::ePipelineCompileRequiredEXT:
				VMI_LOG("Graphics pipeline creation: PipelineCompileRequiredEXT");
				break;
			default: assert(false);
		}
	}
	void create_lighting_pass_graphics_pipeline(DeviceWrapper& deviceWrapper)
	{
		vk::PipelineShaderStageCreateInfo vertStageInfo = vk::PipelineShaderStageCreateInfo()
			.setStage(vk::ShaderStageFlagBits::eVertex)
			.setModule(lighting_vs)
			// entrypoint (one shader module with multiple entry points for different shading stages?)
			.setPName("main")
			.setPSpecializationInfo(nullptr); // constants for optimization

		vk::PipelineShaderStageCreateInfo fragStageInfo = vk::PipelineShaderStageCreateInfo()
			.setStage(vk::ShaderStageFlagBits::eFragment) // fragment == pixel shader in hlsl
			.setModule(lighting_ps)
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
		// 
		// -> for each color attachment
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
		std::array<vk::PipelineColorBlendAttachmentState, 3> gbufferBlendAttachments = {
			colorBlendAttachment, colorBlendAttachment, colorBlendAttachment
		};
		//
		// -> global
		vk::PipelineColorBlendStateCreateInfo colorBlendInfo = vk::PipelineColorBlendStateCreateInfo()
			.setLogicOpEnable(VK_FALSE).setLogicOp(vk::LogicOp::eCopy)
			.setAttachmentCount(gbufferBlendAttachments.size()).setPAttachments(gbufferBlendAttachments.data())
			.setBlendConstants({ 0.0f, 0.0f, 0.0f, 0.0f });

		vk::PipelineDepthStencilStateCreateInfo depthStencilInfo = vk::PipelineDepthStencilStateCreateInfo()
			.setDepthTestEnable(VK_FALSE)
			.setDepthWriteEnable(VK_FALSE)
			.setDepthCompareOp(vk::CompareOp::eLessOrEqual)
			// Depth bounds
			.setDepthBoundsTestEnable(VK_FALSE)
			.setMinDepthBounds(0.0f).setMaxDepthBounds(1.0f)
			// Stencil
			.setStencilTestEnable(VK_FALSE);

		// Pipeline Layout
		std::array<vk::DescriptorSetLayout, 3> layouts = {
			ringBuffer.frames[0].gPosDescSetLayout,
			ringBuffer.frames[0].gColDescSetLayout,
			ringBuffer.frames[0].gNormDescSetLayout
		};
		vk::PipelineLayoutCreateInfo pipelineLayoutInfo = vk::PipelineLayoutCreateInfo()
			.setSetLayoutCount(3).setPSetLayouts(layouts.data())
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
			.setSubpass(1);

		auto result = deviceWrapper.logicalDevice.createGraphicsPipeline(nullptr, graphicsPipelineInfo);
		geometryPassPipeline = result.value;
		switch (result.result)
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
		RingFrame& frame = ringBuffer.frames[iFrame];
		vk::CommandBuffer& commandBuffer = frame.commandBuffer;
		vk::CommandBufferBeginInfo beginInfo = vk::CommandBufferBeginInfo()
			.setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit)
			.setPInheritanceInfo(nullptr);
		commandBuffer.begin(beginInfo);

		// clear color
		std::array<vk::ClearValue, 5> clearValues = { 
			vk::ClearValue(vk::ClearColorValue().setFloat32({ 0.0f, 0.0f, 0.0f, 1.0f })),
			vk::ClearValue(vk::ClearDepthStencilValue().setDepth(1.0f).setStencil(0)),
			vk::ClearValue(vk::ClearColorValue().setFloat32({ 0.0f, 0.0f, 0.0f, 0.0f })),
			vk::ClearValue(vk::ClearColorValue().setFloat32({ 0.0f, 0.0f, 0.0f, 0.0f })),
			vk::ClearValue(vk::ClearColorValue().setFloat32({ 0.0f, 0.0f, 0.0f, 0.0f }))
		};

		vk::RenderPassBeginInfo renderPassBeginInfo = vk::RenderPassBeginInfo()
			.setRenderPass(renderPass)
			.setFramebuffer(frame.swapchainFramebuffer)
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

	// TODO: create render pass wrapper for geometry and lighting pass
private:
	uint32_t nMaxFrames = 2; // max frames in flight

	vma::Allocator allocator;
	SwapchainWrapper swapchainWrapper;
	RingBuffer ringBuffer;

	// todo: shove to render pass wrapper
	vk::ShaderModule vs, ps;
	vk::ShaderModule geometry_vs, geometry_ps;
	vk::ShaderModule lighting_vs, lighting_ps;
	vk::Pipeline geometryPassPipeline;
	vk::Pipeline lightingPassPipeline;

	vk::RenderPass renderPass;
	vk::Pipeline graphicsPipeline;
	vk::PipelineLayout pipelineLayout;
	vk::PipelineCache pipelineCache;

	vk::CommandPool transientCommandPool;
	vk::DescriptorPool imguiDescPool;
	vk::DescriptorPool descPool;

	UniformBufferWrapper<UniformBufferObject> mvpBuffer;
	IndexedGeometry geometry;
};