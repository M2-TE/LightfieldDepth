#pragma once

#include "window.hpp"
#include "device_manager.hpp"

class Application
{
public:
	Application() = default;
	~Application() = default;
	ROF_COPY_MOVE_DELETE(Application)

public:
	void run()
	{
		init();
	}

private:
	void init()
	{
		window.init(dld, debugMessenger);

		auto& instance = window.get_vulkan_instance();
		auto& surface = window.get_vulkan_surface();

		deviceManager.init(instance, surface);
		deviceManager.create_logical_device(device);

		//CreateSwapChain();
		//CreateImageViews();
		//CreateRenderPass();
		//CreateGraphicsPipeline();
		//CreateFramebuffers();
		//CreateCommandPool();
		//CreateCommandBuffers();
		//CreateSyncObjects();
	}

private:
	Window window;
	DeviceManager deviceManager;


	// lazy constants (settings?)
	const int MAX_FRAMES_IN_FLIGHT = 2;

	vk::Device device;
	vk::Queue qGraphics, qPresent;
	vk::SwapchainKHR swapchain;
	std::vector<vk::Image> swapchainImages;
	std::vector<vk::ImageView> swapchainImageViews;
	std::vector<vk::Framebuffer> swapchainFramebuffers;
	vk::Format swapchainImageFormat;
	vk::Extent2D swapchainExtent;
	vk::ShaderModule vertShaderModule;
	vk::ShaderModule fragShaderModule;
	vk::RenderPass renderPass;
	vk::PipelineLayout pipelineLayout;
	vk::Pipeline graphicsPipeline;
	vk::CommandPool commandPool;
	std::vector<vk::CommandBuffer> commandBuffers;

	std::vector<vk::Semaphore> imageAvailableSemaphores;
	std::vector<vk::Semaphore> renderFinishedSemaphores;
	std::vector<vk::Fence> inFlightFences;
	std::vector<vk::Fence> imagesInFlight;
	size_t currentFrame = 0;

	DEBUG_ONLY(vk::DispatchLoaderDynamic dld);
	DEBUG_ONLY(vk::DebugUtilsMessengerEXT debugMessenger);
};