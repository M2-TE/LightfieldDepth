#pragma once

#include "vk_mem_alloc.hpp"
#include "utils/types.hpp"
#include "ring_buffer.hpp"
#include "camera.hpp"
#include "wrappers/imgui_wrapper.hpp"
#include "wrappers/swapchain_wrapper.hpp"
#include "wrappers/shader_wrapper.hpp"
#include "geometry/indexed_geometry.hpp"
#include "render_passes/deferred_renderpass.hpp"
#include "render_passes/swapchain_write.hpp"

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
		allocator.destroy();
	}

	// runtime
	void recreate_KHR(DeviceWrapper& deviceWrapper, Window& window) // TODO use better approach of recreating swapchain using old swapchain pointer
	{
		VMI_LOG("Rebuilding KHR");

		destroy_KHR(deviceWrapper);
		create_KHR(deviceWrapper, window);
	}
	void dump_mem_vma()
	{
		std::string stats = allocator.buildStatsString(true);
		std::ofstream out("vma_stats.json");
		out << stats;
		out.close();
		VMI_LOG("Dumped VMA stats to vma_stats.json");
	}
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
		camera.init(deviceWrapper, allocator, descPool, swapchainWrapper);

		// create deferred render pass
		std::vector<vk::DescriptorSetLayout> geometryPassDescSetLayouts = { camera.get_desc_set_layout() };
		std::vector<vk::DescriptorSetLayout> lightingPassDescSetLayouts = {};
		DeferredRenderpassCreateInfo createInfo = {
			deviceWrapper, swapchainWrapper, allocator, descPool,
			geometryPassDescSetLayouts, lightingPassDescSetLayouts
		};
		deferredRenderpass.init(createInfo);

		swapchainWriteRenderpass.init(deviceWrapper, swapchainWrapper, descPool, deferredRenderpass.get_output_image_view());
	}
	void destroy_KHR(DeviceWrapper& deviceWrapper)
	{
		deferredRenderpass.destroy(deviceWrapper, allocator);
		swapchainWriteRenderpass.destroy(deviceWrapper);
		swapchainWrapper.destroy(deviceWrapper);
		camera.destroy(deviceWrapper, allocator);
	}

	// runtime
	void record_command_buffer(uint32_t iFrame, vk::CommandBuffer& commandBuffer)
	{
		// setting up command buffer
		vk::CommandBufferBeginInfo beginInfo = vk::CommandBufferBeginInfo()
			.setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit)
			.setPInheritanceInfo(nullptr);
		commandBuffer.begin(beginInfo);
		
		// update camera buffer
		camera.update();

		// deferred renderpass
		deferredRenderpass.begin(commandBuffer);
		deferredRenderpass.bind_desc_sets(commandBuffer, camera.get_desc_set());
		geometry.draw(commandBuffer);
		deferredRenderpass.end(commandBuffer);

		// direct write to swapchain image
		swapchainWriteRenderpass.begin(commandBuffer, iFrame);
		swapchainWriteRenderpass.end(commandBuffer);

		// finalize command buffer
		commandBuffer.end();
	}

private:
	uint32_t nMaxFrames = 2; // max frames in flight (minimum for swapchain)

	vma::Allocator allocator;
	DeferredRenderpass deferredRenderpass;
	SwapchainWrite swapchainWriteRenderpass;
	SwapchainWrapper swapchainWrapper;
	ImguiWrapper imguiWrapper;

	RingBuffer<SyncFrameData> syncFrames;
	vk::CommandPool transientCommandPool; // TODO: transfer queue!
	vk::DescriptorPool descPool;

	// scene objects
	Camera camera;
	IndexedGeometry geometry;
};