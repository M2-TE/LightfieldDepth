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
		renderer.destroy(deviceManager);
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

		//ImGui_ImplSDL2_InitForVulkan(nullptr);

		deviceManager.init(window.get_vulkan_instance(), window.get_vulkan_surface());

		renderer.init(deviceManager.get_device_wrapper(), window);
	}
	bool update()
	{
		// Poll for user input (encapsulate in input.hpp?)
		SDL_Event event;
		while (SDL_PollEvent(&event)) {
			switch (event.type) {
				case SDL_QUIT: return false;
				// TODO: handle more cases
				default: break;
			}
		}

		renderer.render(deviceManager);
		SDL_Delay(10); // reduce strain on system for now
		return true;
	}

private:
	Window window;
	DeviceManager deviceManager;
	Renderer renderer;
};