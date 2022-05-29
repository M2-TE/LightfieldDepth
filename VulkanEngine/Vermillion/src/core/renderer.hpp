#pragma once

#include "vk_mem_alloc.hpp"
#include "ring_buffer.hpp"
#include "utils/types.hpp"
#include "wrappers/swapchain_wrapper.hpp"
#include "wrappers/shader_wrapper.hpp"
#include "wrappers/uniform_buffer_wrapper.hpp"
#include "wrappers/render_pass_wrapper.hpp"
#include "geometry/indexed_geometry.hpp"
#include "deferred_rendering/deferred_renderpass.hpp"

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
		create_descriptor_pools(deviceWrapper);
		create_command_pools(deviceWrapper);

		mvpBuffer.allocate(deviceWrapper, allocator, descPool, 0, vk::ShaderStageFlagBits::eVertex, nMaxFrames);
		create_KHR(deviceWrapper, window);

		imgui_init_vulkan(deviceWrapper, window);
		imgui_upload_fonts(deviceWrapper);

		geometry.allocate(allocator, transientCommandPool, deviceWrapper);
	}
	void destroy(DeviceWrapper& deviceWrapper)
	{
		vk::Device& device = deviceWrapper.logicalDevice;

		destroy_KHR(deviceWrapper);

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

	void create_KHR(DeviceWrapper& deviceWrapper, Window& window)
	{
		swapchainWrapper.init(deviceWrapper, window, nMaxFrames);
		ringBuffer.init(deviceWrapper, allocator, swapchainWrapper, nMaxFrames);

		// create deferred render pass
		std::vector<vk::ImageView> swapchainImageViews(nMaxFrames);
		std::vector<vk::ImageView> depthStencilViews(nMaxFrames);
		for (size_t i = 0; i < nMaxFrames; i++) {
			swapchainImageViews[i] = ringBuffer.frames[i].swapchainImageView;
			depthStencilViews[i] = ringBuffer.frames[i].depthStencilView;
		}
		std::vector<vk::DescriptorSetLayout> lightingPassDescSetLayouts(0);
		std::vector<vk::DescriptorSetLayout> geometryPassDescSetLayouts = { mvpBuffer.get_desc_set_layout() };
		DeferredRenderpassCreateInfo createInfo = {
			deviceWrapper, swapchainWrapper,
			allocator, descPool, 
			swapchainImageViews, depthStencilViews,
			geometryPassDescSetLayouts, lightingPassDescSetLayouts,
			nMaxFrames
		};
		deferredRenderpass.init(createInfo);
	}
	void destroy_KHR(DeviceWrapper& deviceWrapper)
	{
		deferredRenderpass.destroy(deviceWrapper, allocator);
		ringBuffer.destroy(deviceWrapper, allocator);
		swapchainWrapper.destroy(deviceWrapper);
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
		info.PipelineCache = nullptr;
		info.DescriptorPool = imguiDescPool;
		info.Subpass = 1;
		info.MinImageCount = (uint32_t) ringBuffer.frames.size(); // TODO: can prolly remove this one
		info.ImageCount = (uint32_t)ringBuffer.frames.size();
		info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;

		ImGui_ImplVulkan_Init(&info, deferredRenderpass.get_render_pass());
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
		// setting up command buffer
		vk::CommandBuffer& commandBuffer = ringBuffer.frames[iFrame].commandBuffer;
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
			.setRenderPass(deferredRenderpass.get_render_pass())
			.setFramebuffer(deferredRenderpass.get_framebuffer(iFrame))
			.setRenderArea(vk::Rect2D({ 0, 0 }, swapchainWrapper.extent))
			// clear value
			.setClearValueCount(clearValues.size()).setPClearValues(clearValues.data());

		// first subpass
		{
			commandBuffer.beginRenderPass(renderPassBeginInfo, vk::SubpassContents::eInline);
			commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, deferredRenderpass.get_geometry_pass());

			commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, deferredRenderpass.get_geometry_pass_layout(), 0, mvpBuffer.get_desc_set(iFrame), {});
			geometry.draw(commandBuffer);
		}

		// second subpass
		{
			commandBuffer.nextSubpass(vk::SubpassContents::eInline);
			commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, deferredRenderpass.get_lighting_pass());

			std::array<vk::DescriptorSet, 1> descSets = { deferredRenderpass.get_descriptor_set(iFrame) };
			commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, deferredRenderpass.get_lighting_pass_layout(), 0, deferredRenderpass.get_descriptor_set(iFrame), {});
			commandBuffer.draw(3, 1, 0, 0);

			ImGui::Render();
			ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), commandBuffer);
		}


		commandBuffer.endRenderPass();
		commandBuffer.end();
	}

private:
	uint32_t nMaxFrames = 2; // max frames in flight

	DeferredRenderpass deferredRenderpass;
	SwapchainWrapper swapchainWrapper;
	vma::Allocator allocator;
	RingBuffer ringBuffer;

	vk::CommandPool transientCommandPool;
	vk::DescriptorPool imguiDescPool;
	vk::DescriptorPool descPool;

	UniformBufferWrapper<UniformBufferObject> mvpBuffer;
	IndexedGeometry geometry;
};