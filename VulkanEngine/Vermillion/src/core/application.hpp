#pragma once

#include "window.hpp"
#include "device_manager.hpp"
#include "renderer.hpp"

class Application
{
public:
	Application()
	{
		init();
	}
	~Application()
	{
		deviceManager.destroy();
		window.destroy();
	}
	ROF_COPY_MOVE_DELETE(Application)

public:
	void run()
	{
		bool bRunning = true;
		while (bRunning) {
			// Poll for user input (could move to window.hpp)
			SDL_Event event;
			while (SDL_PollEvent(&event)) {

				switch (event.type) {

				case SDL_QUIT:
					bRunning = false;
					break;

				default:
					// Do nothing.
					break;
				}
			}

			//DrawFrame();
			//SDL_Delay(10);
		}

		deviceManager.get_logical_device().waitIdle();
	}

private:
	void init()
	{
		window.init();

		deviceManager.init(window.get_vulkan_instance(), window.get_vulkan_surface());
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