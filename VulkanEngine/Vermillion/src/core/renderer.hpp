#pragma once

#include "vk_mem_alloc.hpp"
#include "utils/types.hpp"
#include "wrappers/swapchain_wrapper.hpp"
#include "wrappers/shader_wrapper.hpp"
#include "wrappers/descriptor_wrapper.hpp"
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

		create_command_pools(deviceWrapper);
		create_command_buffers(deviceWrapper);

		geometry.allocate(allocator, transientCommandPool, deviceWrapper);

		descriptorWrapper.init(deviceWrapper);
		uint32_t nFrames = swapchainWrapper.MAX_FRAMES_IN_FLIGHT;
		mvpBuffer.allocate(allocator, nFrames);
		descriptorWrapper.add_uniform_buffer(mvpBuffer, 0, vk::ShaderStageFlagBits::eVertex);
		descriptorWrapper.update(deviceWrapper, nFrames);

		create_shader_modules(deviceWrapper);
		create_descriptor_set_layout(deviceWrapper); // TODO: move into swapchain build/rebuild?
		create_descriptor_pools(deviceWrapper);
		create_descriptor_sets(deviceWrapper);

		create_KHR(deviceWrapper, window);

		imgui_init_vulkan(deviceWrapper, window);
		imgui_upload_fonts(deviceWrapper);
	}
	void recreate_KHR(DeviceWrapper& deviceWrapper, Window& window) // TODO use better approach of recreating swapchain using old swapchain pointer
	{
		VMI_LOG("Rebuilding KHR");

		destroy_KHR(deviceWrapper);
		create_KHR(deviceWrapper, window);
	}
	void destroy(DeviceWrapper& deviceWrapper)
	{
		vk::Device& device = deviceWrapper.logicalDevice;

		destroy_KHR(deviceWrapper);

		for (size_t i = 0; i < swapchainWrapper.MAX_FRAMES_IN_FLIGHT; i++) {
			device.destroyCommandPool(commandPools[i]);
		}
		device.destroyCommandPool(transientCommandPool);

		device.destroyShaderModule(vs);
		device.destroyShaderModule(ps);

		device.destroyDescriptorPool(imguiDescPool);
		device.destroyDescriptorPool(descPool);
		device.destroyDescriptorSetLayout(descSetLayout);

		ImGui_ImplVulkan_Shutdown();

		geometry.deallocate(allocator);
		mvpBuffer.deallocate(allocator);
		descriptorWrapper.destroy(deviceWrapper);
		allocator.destroy();
	}

	void render(DeviceWrapper& deviceWrapper)
	{
		uint32_t iImage = swapchainWrapper.acquire_image(deviceWrapper);
		uint32_t iFrame = (uint32_t)swapchainWrapper.currentFrame;

		deviceWrapper.logicalDevice.resetCommandPool(commandPools[iFrame]);
		update_uniform_buffer(deviceWrapper, iFrame);
		record_command_buffer(iFrame, iImage);

		swapchainWrapper.present(deviceWrapper, commandBuffers[iFrame], iImage);
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

	// for swapchain builds/rebuilds
	void create_KHR(DeviceWrapper& deviceWrapper, Window& window)
	{
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
			.setSetLayoutCount(1)
			.setSetLayouts(descSetLayout)
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

	void create_shader_modules(DeviceWrapper& deviceWrapper)
	{
		vs = create_shader_module(deviceWrapper, shader_vs, shader_vs_size);
		ps = create_shader_module(deviceWrapper, shader_ps, shader_ps_size);
	}

	// TODO: move to uniform buffer wrapper?
	void create_descriptor_set_layout(DeviceWrapper& deviceWrapper)
	{
		vk::DescriptorSetLayoutBinding layoutBinding = vk::DescriptorSetLayoutBinding()
			.setBinding(0)
			.setDescriptorType(vk::DescriptorType::eUniformBuffer)
			.setDescriptorCount(1)
			.setStageFlags(vk::ShaderStageFlagBits::eVertex)
			.setPImmutableSamplers(nullptr);

		vk::DescriptorSetLayoutCreateInfo createInfo = vk::DescriptorSetLayoutCreateInfo()
			.setBindingCount(1)
			.setPBindings(&layoutBinding);

		descSetLayout = deviceWrapper.logicalDevice.createDescriptorSetLayout(createInfo);
	}
	void create_descriptor_pools(DeviceWrapper& deviceWrapper)
	{
		vk::DescriptorPoolSize poolSize = vk::DescriptorPoolSize()
			.setType(vk::DescriptorType::eUniformBuffer)
			.setDescriptorCount(swapchainWrapper.MAX_FRAMES_IN_FLIGHT);

		vk::DescriptorPoolCreateInfo info = vk::DescriptorPoolCreateInfo()
			//.setFlags()
			.setMaxSets(swapchainWrapper.MAX_FRAMES_IN_FLIGHT)
			.setPoolSizeCount(1)
			.setPPoolSizes(&poolSize);
		descPool = deviceWrapper.logicalDevice.createDescriptorPool(info);


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

		info = vk::DescriptorPoolCreateInfo()
			.setFlags(vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet)
			.setMaxSets(descCountImgui * (uint32_t)poolSizes.size())
			.setPoolSizeCount((uint32_t)poolSizes.size())
			.setPPoolSizes(poolSizes.data());

		imguiDescPool = deviceWrapper.logicalDevice.createDescriptorPool(info);
	}
	void create_descriptor_sets(DeviceWrapper& deviceWrapper)
	{
		std::vector<vk::DescriptorSetLayout> layouts(swapchainWrapper.MAX_FRAMES_IN_FLIGHT, descSetLayout);
		vk::DescriptorSetAllocateInfo allocInfo = vk::DescriptorSetAllocateInfo()
			.setDescriptorPool(descPool)
			.setDescriptorSetCount(swapchainWrapper.MAX_FRAMES_IN_FLIGHT)
			.setPSetLayouts(layouts.data());

		descSets = deviceWrapper.logicalDevice.allocateDescriptorSets(allocInfo);

		size_t n = swapchainWrapper.MAX_FRAMES_IN_FLIGHT;
		std::vector<vk::DescriptorBufferInfo> descBufferInfos(n);
		std::vector<vk::WriteDescriptorSet> descBufferWrites(n);
		for (size_t i = 0; i < swapchainWrapper.MAX_FRAMES_IN_FLIGHT; i++) {
			descBufferInfos[i] = vk::DescriptorBufferInfo()
				.setBuffer(mvpBuffer.get_buffer(i))
				.setOffset(0)
				.setRange(mvpBuffer.get_buffer_size());

			descBufferWrites[i] = vk::WriteDescriptorSet()
				.setDstSet(descSets[i])
				.setDstBinding(0)
				.setDstArrayElement(0)
				.setDescriptorType(vk::DescriptorType::eUniformBuffer)
				.setDescriptorCount(1)
				//
				.setPBufferInfo(&descBufferInfos[i])
				.setPImageInfo(nullptr)
				.setPTexelBufferView(nullptr);

		}
		deviceWrapper.logicalDevice.updateDescriptorSets(descBufferWrites, {});
	}

	void create_command_pools(DeviceWrapper& deviceWrapper)
	{
		vk::CommandPoolCreateInfo commandPoolInfo;

		commandPoolInfo = vk::CommandPoolCreateInfo()
			.setQueueFamilyIndex(deviceWrapper.iQueue);
		commandPools.resize(swapchainWrapper.MAX_FRAMES_IN_FLIGHT);
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
		commandBuffers.resize(swapchainWrapper.MAX_FRAMES_IN_FLIGHT);
		for (uint32_t i = 0; i < commandPools.size(); i++) {
			vk::CommandBufferAllocateInfo commandBufferInfo = vk::CommandBufferAllocateInfo()
				.setCommandPool(commandPools[i])
				.setLevel(vk::CommandBufferLevel::ePrimary) // secondary are used by primary command buffers for e.g. common operations
				.setCommandBufferCount(1);
			commandBuffers[i] = deviceWrapper.logicalDevice.allocateCommandBuffers(commandBufferInfo)[0];
		}
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
		info.MinImageCount = (uint32_t) swapchainWrapper.images.size();
		info.ImageCount = (uint32_t) swapchainWrapper.images.size();
		info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;

		ImGui_ImplVulkan_Init(&info, renderPass);
	}
	void imgui_upload_fonts(DeviceWrapper& deviceWrapper)
	{
		vk::CommandBuffer commandBuffer = commandBuffers[0];
		deviceWrapper.logicalDevice.resetCommandPool(commandPools[0]);

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
	void record_command_buffer(uint32_t iFrame, uint32_t iImage)
	{
		vk::CommandBuffer& commandBuffer = commandBuffers[iFrame];


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
			.setFramebuffer(swapchainWrapper.framebuffers[iImage])
			.setRenderArea(vk::Rect2D({ 0, 0 }, swapchainWrapper.extent))
			// clear value
			.setClearValueCount(1)
			.setPClearValues(&clearValue);

		commandBuffer.beginRenderPass(renderPassBeginInfo, vk::SubpassContents::eInline);
		commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, graphicsPipeline);

		commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelineLayout, 0, descSets[iFrame], {});
		geometry.draw(commandBuffer);


		ImGui::Render();
		ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), commandBuffer);

		commandBuffer.endRenderPass();

		commandBuffer.end();
	}

private:
	// TODO: create a wrapper for all per-frame and per-image objects (all the std::vectors)

	vma::Allocator allocator;
	SwapchainWrapper swapchainWrapper;
	DescriptorWrapper descriptorWrapper;
	UniformBufferWrapper<UniformBufferObject> mvpBuffer;

	vk::ShaderModule vs, ps;
	vk::RenderPass renderPass;
	vk::Pipeline graphicsPipeline;
	vk::PipelineLayout pipelineLayout;
	vk::PipelineCache pipelineCache;

	vk::DescriptorPool imguiDescPool;
	vk::DescriptorPool descPool;
	vk::DescriptorSetLayout descSetLayout; // for cbuffer
	std::vector<vk::DescriptorSet> descSets;

	vk::CommandPool transientCommandPool;
	std::vector<vk::CommandPool> commandPools;
	std::vector<vk::CommandBuffer> commandBuffers;

	IndexedGeometry geometry;
};