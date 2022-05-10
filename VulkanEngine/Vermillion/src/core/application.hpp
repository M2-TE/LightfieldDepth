#pragma once

#include "window.hpp"
#include "device_manager.hpp"
#include "renderer.hpp"

class Application
{
public:
	Application()
	{
		window.init();
		deviceManager.init(window.get_vulkan_instance(), window.get_vulkan_surface());
		renderer.init(deviceManager.get_device_wrapper(), window);
	}
	~Application()
	{
		renderer.destroy(deviceManager.get_device_wrapper());
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
	bool update()
	{
		ImGui_ImplVulkan_NewFrame();
		ImGui_ImplSDL2_NewFrame();
		ImGui::NewFrame();

		// Poll for user input (encapsulate in input.hpp?)
		SDL_Event event;
		while (SDL_PollEvent(&event)) {

			bool b = ImGui_ImplSDL2_ProcessEvent(&event);
			// TODO: handle b

			switch (event.type) {
				case SDL_QUIT: return false;
				// TODO: handle more cases
				default: break;
			}
		}

		ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);

		renderer.render(deviceManager);

		return true;
	}

private:
	Window window;
	DeviceManager deviceManager;
	Renderer renderer;
};