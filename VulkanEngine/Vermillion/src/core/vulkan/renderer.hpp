#pragma once

#include "vk_mem_alloc.hpp"
#include "utils/types.hpp"
#include "scene_objects/camera.hpp"
#include "wrappers/imgui_wrapper.hpp"
#include "wrappers/swapchain_wrapper.hpp"
#include "wrappers/shader_wrapper.hpp"
#include "render_passes/lightfield/lightfield.hpp"
#include "render_passes/lightfield/forward_renderpass.hpp"
#include "render_passes/lightfield/gradients_renderpass.hpp"
#include "render_passes/lightfield/disparity_renderpass.hpp"
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
		VMI_LOG("[Initializing] Renderer...");
		create_vma_allocator(deviceWrapper, window);
		create_descriptor_pools(deviceWrapper);
		create_command_pools(deviceWrapper);

		create_KHR(deviceWrapper, window);
		syncFrames.set_size(swapchainWrapper.nImages).init(deviceWrapper);

		imguiWrapper.init(deviceWrapper, window, swapchainWriteRenderpass.get_render_pass(), syncFrames);
	}
	void destroy(DeviceWrapper& deviceWrapper, entt::registry& reg)
	{
		vk::Device& device = deviceWrapper.logicalDevice;
		
		destroy_KHR(deviceWrapper);

		device.destroyCommandPool(transientCommandPool);
		device.destroyDescriptorPool(descPool);

		syncFrames.destroy(deviceWrapper);

		imguiWrapper.destroy(deviceWrapper);
		ImGui_ImplVulkan_Shutdown();

		deallocate_entities(deviceWrapper, reg);
		allocator.destroy();
	}

	// runtime
	void recreate_KHR(DeviceWrapper& deviceWrapper, Window& window, bool bForceRebuild) // TODO use better approach of recreating swapchain using old swapchain pointer
	{
		// check if resize is necessary
		Sint32 w, h;
		SDL_GetWindowSize(window.get_window(), &w, &h);
		if (w != swapchainWrapper.extent.width || h != swapchainWrapper.extent.height || bForceRebuild) {

			VMI_LOG("Rebuilding KHR");
			deviceWrapper.logicalDevice.waitIdle();
			destroy_KHR(deviceWrapper);
			create_KHR(deviceWrapper, window);
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
	void handle_allocations(DeviceWrapper& deviceWrapper, entt::registry& reg)
	{
		deallocate_entities(deviceWrapper, reg);
		allocate_entities(deviceWrapper, reg);
	}
	void render(DeviceWrapper& deviceWrapper, entt::registry& reg, uint32_t iRenderMode)
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
				case vk::Result::eErrorOutOfDateKHR: VMI_ERR("Swapchain (Image Acquisition): KHR out of date."); break;
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
			record_command_buffer(reg, syncFrame.commandBuffer, iFrame, iRenderMode);
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
				case vk::Result::eErrorOutOfDateKHR: VMI_ERR("Swapchain (Present): KHR out of date."); break;
				default: assert(false);
			}
		}
	}

	// TESTING
	void handle_input(Input& input)
	{
		camera.handle_input(input);
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
		vk::DescriptorPoolCreateFlags flags;

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
		swapchainWrapper.init(deviceWrapper, window);
		camera.init(deviceWrapper, allocator, descPool, swapchainWrapper);

		// 9 camera views, along with disparity and gradient maps
		LightfieldCreateInfo lightfieldInfo = { deviceWrapper, swapchainWrapper, allocator, descPool, transientCommandPool };
		lightfield.init(lightfieldInfo);

		// create lightfield and the renderpass that writes to it
		ForwardRenderpassCreateInfo forwardInfo = { deviceWrapper, swapchainWrapper, allocator, descPool, lightfield };
		forwardRenderpass.init(forwardInfo);

		GradientsRenderpassCreateInfo gradientsInfo = { deviceWrapper, swapchainWrapper, allocator, descPool, lightfield };
		gradientsRenderpass.init(gradientsInfo);

		DisparityRenderpassCreateInfo disparityInfo = { deviceWrapper, swapchainWrapper, allocator, descPool, lightfield };
		disparityRenderpass.init(disparityInfo);

		swapchainWriteRenderpass.init(deviceWrapper, swapchainWrapper, descPool, lightfield.disparityImageView);
	}
	void destroy_KHR(DeviceWrapper& deviceWrapper)
	{
		camera.destroy(deviceWrapper, allocator);

		lightfield.destroy(deviceWrapper, allocator);
		forwardRenderpass.destroy(deviceWrapper, allocator);
		gradientsRenderpass.destroy(deviceWrapper);
		disparityRenderpass.destroy(deviceWrapper);
		swapchainWriteRenderpass.destroy(deviceWrapper);

		swapchainWrapper.destroy(deviceWrapper);
	}
	
	// runtime
	void allocate_entities(DeviceWrapper& deviceWrapper, entt::registry& reg)
	{
		systems::Geometry::allocate(reg, deviceWrapper, allocator, transientCommandPool);
	}
	void deallocate_entities(DeviceWrapper& deviceWrapper, entt::registry& reg)
	{
		systems::Geometry::deallocate(reg, allocator);
	}
	void record_command_buffer(entt::registry& reg, vk::CommandBuffer& commandBuffer, uint32_t iFrame, uint32_t iRenderMode)
	{
		// setting up command buffer
		vk::CommandBufferBeginInfo beginInfo = vk::CommandBufferBeginInfo()
			.setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit)
			.setPInheritanceInfo(nullptr);
		commandBuffer.begin(beginInfo);

		// update camera buffer (and other buffers later)
		//camera.update();

		// writing to lightfield (9 cams)
		//for (auto i = 0u; i < 9; i++) {
		//	forwardRenderpass.begin(commandBuffer, i);
		//	forwardRenderpass.bind_desc_sets(commandBuffer, camera.get_desc_set(), i);
		//	systems::Geometry::bind(reg, commandBuffer);
		//	forwardRenderpass.end(commandBuffer);
		//}

		gradientsRenderpass.execute(commandBuffer, iRenderMode);
		disparityRenderpass.execute(commandBuffer);

		// direct write to swapchain image
		swapchainWriteRenderpass.execute(commandBuffer, iFrame);

		// finalize command buffer
		commandBuffer.end();
	}

private:
	vma::Allocator allocator;
	SwapchainWrapper swapchainWrapper;
	ImguiWrapper imguiWrapper;

	Lightfield lightfield;
	ForwardRenderpass forwardRenderpass;
	GradientsRenderpass gradientsRenderpass;
	DisparityRenderpass disparityRenderpass;
	SwapchainWrite swapchainWriteRenderpass;

	RingBuffer<SyncFrameData> syncFrames;
	vk::CommandPool transientCommandPool; // TODO: transfer queue!
	vk::DescriptorPool descPool;

	// scene objects
	Camera camera;
};