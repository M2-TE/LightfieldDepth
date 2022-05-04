#pragma once

#include "window.hpp"
#include "device_manager.hpp"
#include "renderer.hpp"

class Application
{
public:
	Application() = default;
	~Application()
	{
		deviceManager.destroy();
		window.destroy();
	}
	ROF_COPY_MOVE_DELETE(Application)

public:
	void run()
	{
		init();

		for (;;) {
			update();
			break;
		}
	}

private:
	void init()
	{
		window.init();

		auto& instance = window.get_vulkan_instance();
		auto& surface = window.get_vulkan_surface();

		deviceManager.init(instance, surface);
		deviceManager.create_logical_device();

		//CreateSwapChain();
		//CreateImageViews();
		//CreateRenderPass();
		//CreateGraphicsPipeline();
		//CreateFramebuffers();
		//CreateCommandPool();
		//CreateCommandBuffers();
		//CreateSyncObjects();
	}
	void update()
	{
		// TODO
	}

private:
	Window window;
	DeviceManager deviceManager;
	Renderer renderer;
};