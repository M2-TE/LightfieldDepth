#pragma once

#include "window.hpp"
#include "device_manager.hpp"
#include "renderer.hpp"
#include "Input.hpp"

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
		bool bRendering = true;
		bool bResizing = false;

		ImGui_ImplVulkan_NewFrame();
		ImGui_ImplSDL2_NewFrame();
		ImGui::NewFrame();

		SDL_Event event;
		while (SDL_PollEvent(&event)) {

			ImGui_ImplSDL2_ProcessEvent(&event);

			switch (event.type) {
				case SDL_QUIT: return false;

				case SDL_MOUSEBUTTONDOWN:
				case SDL_MOUSEBUTTONUP: input.register_mouse_button_event(event.button); break;
				case SDL_MOUSEMOTION: input.register_mouse_motion_event(event.motion); break;

				case SDL_KEYUP:
				case SDL_KEYDOWN: input.register_keyboard_event(event.key); break;

				case SDL_WINDOWEVENT: {
					switch (event.window.event) {
						case SDL_WINDOWEVENT_RESIZED: 
							SDL_Log("Window %d resized to %dx%d", event.window.windowID, event.window.data1, event.window.data2);
							deviceManager.get_logical_device().waitIdle();
							SDL_SetWindowSize(window.get_window(), event.window.data1, event.window.data2);
							bResizing = true;
							break;

						case SDL_WINDOWEVENT_FOCUS_LOST:
						case SDL_WINDOWEVENT_MINIMIZED: bRendering = false; break;

						case SDL_WINDOWEVENT_RESTORED: bResizing = true; //fallthrough
						case SDL_WINDOWEVENT_FOCUS_GAINED: bRendering = true; break;
					}
				}
			}
		}

		// TODO: move this to a higher-level class?
		if (input.keysPressed.count(SDLK_F11)) {
			deviceManager.get_logical_device().waitIdle();
			//SDL_SetWindowFullscreen(window.get_window(), SDL_WINDOW_FULLSCREEN); // crash when tabbing out, need to investigate
			SDL_SetWindowFullscreen(window.get_window(), SDL_WINDOW_FULLSCREEN_DESKTOP); // borderless
			bResizing = true;
		}
		if (input.keysPressed.count(SDLK_F10)) {
			renderer.dump_mem_vma();
		}

		input.flush();

		ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);

		if (bResizing = false) resize();
		if (bRendering) render();
		else stall();

		return true;
	}

	void resize()
	{
		deviceManager.get_logical_device().waitIdle();
		renderer.recreate_KHR(deviceManager.get_device_wrapper(), window);
	}
	void render()
	{
		renderer.render(deviceManager.get_device_wrapper());
	}
	void stall()
	{
		ImGui::EndFrame(); // manually end imgui frame
		SDL_Delay(200); // slow down update loop while no rendering occurs
	}

private:
	Window window;
	DeviceManager deviceManager;
	Renderer renderer;
	Input input;
};