#pragma once

#include "vk_mem_alloc.hpp"
#include "utils/types.hpp"
#include "ring_buffer.hpp"
#include "camera.hpp"
// wrappers
#include "wrappers/imgui_wrapper.hpp"
#include "wrappers/swapchain_wrapper.hpp"
#include "wrappers/shader_wrapper.hpp"
#include "wrappers/uniform_buffer_wrapper.hpp"
#include "wrappers/render_pass_wrapper.hpp"
// other
#include "geometry/indexed_geometry.hpp"
#include "render_passes/deferred_renderpass.hpp"

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
		create_vma_allocator(deviceWrapper, window);
		create_descriptor_pools(deviceWrapper);
		create_command_pools(deviceWrapper);

		syncFrames.set_size(nMaxFrames).init(deviceWrapper);

		mvpBuffer.allocate(deviceWrapper, allocator, descPool, 0, vk::ShaderStageFlagBits::eVertex, nMaxFrames);
		create_KHR(deviceWrapper, window);

		imguiWrapper.init(deviceWrapper, window, deferredRenderpass.get_render_pass(), syncFrames);

		geometry.allocate(allocator, transientCommandPool, deviceWrapper);
	}
	void destroy(DeviceWrapper& deviceWrapper)
	{
		vk::Device& device = deviceWrapper.logicalDevice;

		destroy_KHR(deviceWrapper);

		device.destroyCommandPool(transientCommandPool);
		device.destroyDescriptorPool(descPool);

		syncFrames.destroy(deviceWrapper);

		imguiWrapper.destroy(deviceWrapper);
		ImGui_ImplVulkan_Shutdown();

		geometry.deallocate(allocator);
		mvpBuffer.deallocate(deviceWrapper, allocator);
		allocator.destroy();
	}
	void destroy_KHR(DeviceWrapper& deviceWrapper)
	{
		deferredRenderpass.destroy(deviceWrapper, allocator);
		swapchainWrapper.destroy(deviceWrapper);
	}
	void recreate_KHR(DeviceWrapper& deviceWrapper, Window& window) // TODO use better approach of recreating swapchain using old swapchain pointer
	{
		VMI_LOG("Rebuilding KHR");

		destroy_KHR(deviceWrapper);
		create_KHR(deviceWrapper, window);
	}

	// runtime
	void render(DeviceWrapper& deviceWrapper)
	{
		// get next frame of sync objects
		auto& syncFrame = syncFrames.get_next();

		uint32_t iFrame;
		// Acquire image
		{
			vk::ResultValue imgResult = deviceWrapper.logicalDevice.acquireNextImageKHR(swapchainWrapper.swapchain, UINT64_MAX, syncFrame.imageAvailable);
			switch (imgResult.result) {
				case vk::Result::eSuccess: break;
				case vk::Result::eSuboptimalKHR: VMI_LOG("Suboptimal image acquisition."); break;
				case vk::Result::eErrorOutOfDateKHR: VMI_ERR("Swapchain: KHR out of date.");
				default: assert(false);
			}
			iFrame = imgResult.value;
		}

		// Render (record)
		{
			// wait for fence of fetched frame before rendering to it
			vk::Result result = deviceWrapper.logicalDevice.waitForFences(syncFrame.commandBufferFence, VK_TRUE, UINT64_MAX);
			if (result != vk::Result::eSuccess) assert(false);
			// and reset it
			deviceWrapper.logicalDevice.resetFences(syncFrame.commandBufferFence);

			// reset command pool and then record into it (using command buffer)
			deviceWrapper.logicalDevice.resetCommandPool(syncFrame.commandPool);
			update_uniform_buffer(deviceWrapper, iFrame);
			record_command_buffer(iFrame, syncFrame.commandBuffer);
		}

		// Render (submit)
		{
			vk::PipelineStageFlags waitStages = vk::PipelineStageFlagBits::eColorAttachmentOutput;
			vk::SubmitInfo submitInfo = vk::SubmitInfo()
				.setPWaitDstStageMask(&waitStages)
				// semaphores
				.setWaitSemaphoreCount(1).setPWaitSemaphores(&syncFrame.imageAvailable)
				.setSignalSemaphoreCount(1).setPSignalSemaphores(&syncFrame.renderFinished)
				// command buffers
				.setCommandBufferCount(1).setPCommandBuffers(&syncFrame.commandBuffer);

			deviceWrapper.queue.submit(submitInfo, syncFrame.commandBufferFence);
		}

		// Present
		{
			vk::PresentInfoKHR presentInfo = vk::PresentInfoKHR()
				.setPImageIndices(&iFrame)
				// semaphores
				.setWaitSemaphoreCount(1).setPWaitSemaphores(&syncFrame.renderFinished)
				// swapchains
				.setSwapchainCount(1).setPSwapchains(&swapchainWrapper.swapchain);
			vk::Result result = deviceWrapper.queue.presentKHR(&presentInfo);
			switch (result) {
				case vk::Result::eSuccess: break;
				case vk::Result::eErrorOutOfDateKHR: VMI_ERR("Swapchain: KHR out of date.");
				default: assert(false);
			}
		}
	}
	void dump_mem_vma()
	{
		std::string stats = allocator.buildStatsString(true);
		std::ofstream out("vma_stats.json");
		out << stats;
		out.close();
		VMI_LOG("Dumped VMA stats to vma_stats.json");
	}

private:
	void create_vma_allocator(DeviceWrapper& deviceWrapper, Window& window)
	{
		vma::AllocatorCreateInfo info = vma::AllocatorCreateInfo()
			.setPhysicalDevice(deviceWrapper.physicalDevice)
			.setDevice(deviceWrapper.logicalDevice)
			.setInstance(window.get_vulkan_instance())
			.setVulkanApiVersion(VK_API_VERSION_1_1)
			.setFlags(vma::AllocatorCreateFlagBits::eKhrDedicatedAllocation);

		allocator = vma::createAllocator(info);
	}
	void create_descriptor_pools(DeviceWrapper& deviceWrapper)
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

		// create deferred render pass
		std::vector<vk::DescriptorSetLayout> geometryPassDescSetLayouts = { mvpBuffer.get_desc_set_layout() };
		std::vector<vk::DescriptorSetLayout> lightingPassDescSetLayouts = {};
		DeferredRenderpassCreateInfo createInfo = {
			deviceWrapper, swapchainWrapper, allocator, descPool,
			geometryPassDescSetLayouts, lightingPassDescSetLayouts,
			nMaxFrames
		};
		deferredRenderpass.init(createInfo);
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
	void record_command_buffer(uint32_t iFrame, vk::CommandBuffer& commandBuffer)
	{
		// setting up command buffer
		vk::CommandBufferBeginInfo beginInfo = vk::CommandBufferBeginInfo()
			.setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit)
			.setPInheritanceInfo(nullptr);
		commandBuffer.begin(beginInfo);

		// clear color
		std::array<vk::ClearValue, 5> clearValues = { 
			vk::ClearValue(vk::ClearColorValue().setFloat32({ 0.0f, 0.0f, 0.0f, 0.0f })),
			vk::ClearValue(vk::ClearColorValue().setFloat32({ 0.0f, 0.0f, 0.0f, 0.0f })),
			vk::ClearValue(vk::ClearColorValue().setFloat32({ 0.0f, 0.0f, 0.0f, 0.0f })),
			vk::ClearValue(vk::ClearColorValue().setFloat32({ 0.0f, 0.0f, 0.0f, 0.0f })),
			vk::ClearValue(vk::ClearDepthStencilValue().setDepth(1.0f).setStencil(0))
		};

		vk::RenderPassBeginInfo renderPassBeginInfo = vk::RenderPassBeginInfo()
			.setRenderPass(deferredRenderpass.get_render_pass())
			.setFramebuffer(deferredRenderpass.get_framebuffer())
			.setRenderArea(vk::Rect2D({ 0, 0 }, swapchainWrapper.extent))
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

			commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, deferredRenderpass.get_lighting_pass_layout(), 0, deferredRenderpass.get_descriptor_set(), {});
			commandBuffer.draw(3, 1, 0, 0);

			ImGui::Render();
			ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), commandBuffer);
		}
		commandBuffer.endRenderPass();

		// blit to swapchain image (temp workaround)
		{
			// TODO: dont create this again every frame, its unnecessary
			vk::ImageSubresourceLayers layers = vk::ImageSubresourceLayers()
				.setAspectMask(vk::ImageAspectFlagBits::eColor)
				.setLayerCount(1).setBaseArrayLayer(0)
				.setMipLevel(0);
			
			vk::Offset3D offset = vk::Offset3D(swapchainWrapper.extent.width, swapchainWrapper.extent.height, 1);
			vk::ImageBlit region = vk::ImageBlit()
				.setSrcOffsets({ vk::Offset3D(0, 0, 0), offset }) // TODO: set actual size
				.setSrcSubresource(layers)
				.setDstOffsets({ vk::Offset3D(0, 0, 0), offset })
				.setDstSubresource(layers);

			commandBuffer.blitImage(
				// src
				deferredRenderpass.get_output_image(),
				vk::ImageLayout::eTransferSrcOptimal,
				// dst
				swapchainWrapper.images[iFrame],
				vk::ImageLayout::eTransferDstOptimal,
				region, 
				vk::Filter::eLinear);

		}


		commandBuffer.end();
	}

private:
	uint32_t nMaxFrames = 2; // max frames in flight (minimum for swapchain)

	vma::Allocator allocator;
	DeferredRenderpass deferredRenderpass;
	SwapchainWrapper swapchainWrapper;
	ImguiWrapper imguiWrapper;

	RingBuffer<SyncFrameData> syncFrames;
	vk::CommandPool transientCommandPool;
	vk::DescriptorPool descPool;

	UniformBufferWrapper<UniformBufferObject> mvpBuffer;
	IndexedGeometry geometry;
};