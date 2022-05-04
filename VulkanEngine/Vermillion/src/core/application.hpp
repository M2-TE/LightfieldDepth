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
		while (update()) {}
		deviceManager.get_logical_device().waitIdle();
	}

private:
	void init()
	{
		window.init();

		deviceManager.init(window.get_vulkan_instance(), window.get_vulkan_surface());
		deviceManager.create_logical_device();

		renderer.init(deviceManager, window);
	}
	bool update()
	{
		// Poll for user input (encapsulate in input.hpp?)
		SDL_Event event;
		while (SDL_PollEvent(&event)) {
			switch (event.type) {
				case SDL_QUIT: return false;
				default: break;
			}
		}

		//DrawFrame();
		SDL_Delay(10); // reduce strain on system for now
		return true;
	}

private:
	Window window;
	DeviceManager deviceManager;
	Renderer renderer;
};