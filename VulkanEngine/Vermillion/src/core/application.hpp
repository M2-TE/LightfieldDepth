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
		deviceManager.get_logical_device().waitIdle();

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

			ImGui_ImplSDL2_ProcessEvent(&event);

			switch (event.type) {
				case SDL_QUIT: return false;

				case SDL_KEYUP:
				case SDL_KEYDOWN: {
					if (event.key.repeat > 0) break;
					switch (event.key.keysym.sym) {
						case SDLK_F11: {
							if (event.key.type == SDL_KEYDOWN) {
								deviceManager.get_logical_device().waitIdle();
								SDL_SetWindowFullscreen(window.get_window(), SDL_WINDOW_FULLSCREEN);
								//SDL_SetWindowFullscreen(window.get_window(), SDL_WINDOW_FULLSCREEN_DESKTOP); // borderless
								bSwapchainRebuildQueued = true;
							}
						}
					}
				}

				case SDL_WINDOWEVENT: {
					switch (event.window.event) {
						case SDL_WINDOWEVENT_RESIZED: {
							SDL_Log("Window %d resized to %dx%d", event.window.windowID, event.window.data1, event.window.data2);
							
							deviceManager.get_logical_device().waitIdle();
							SDL_SetWindowSize(window.get_window(), event.window.data1, event.window.data2);
							bSwapchainRebuildQueued = true;
							break;
						}

						case SDL_WINDOWEVENT_FOCUS_LOST:
						case SDL_WINDOWEVENT_MINIMIZED: {
							bRendering = false;
							break;
						}

						case SDL_WINDOWEVENT_RESTORED: bSwapchainRebuildQueued = true;
						case SDL_WINDOWEVENT_FOCUS_GAINED:
						{
							bRendering = true;
							break;
						}
					}
				}
			}
		}

		ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);

		if (bSwapchainRebuildQueued) {
			deviceManager.get_logical_device().waitIdle();
			renderer.recreate_KHR(deviceManager.get_device_wrapper(), window);
			bSwapchainRebuildQueued = false;
		}

		if (bRendering) renderer.render(deviceManager.get_device_wrapper());
		else {
			ImGui::EndFrame(); // manually end imgui frame
			SDL_Delay(200); // slow down update loop while no rendering occurs
		}

		return true;
	}

private:
	Window window;
	DeviceManager deviceManager;
	Renderer renderer;

	bool bRendering = true;
	bool bSwapchainRebuildQueued = false;
};