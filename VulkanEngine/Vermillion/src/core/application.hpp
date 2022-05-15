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

			bool b = ImGui_ImplSDL2_ProcessEvent(&event);
			// TODO: handle b

			switch (event.type) {
				case SDL_QUIT: return false;
				case SDL_WINDOWEVENT: { // TODO: handle swapchain recreation EXPLICITLY from here
					switch (event.window.event) {
						case SDL_WINDOWEVENT_SHOWN: SDL_Log("Window %d shown", event.window.windowID); break;
						case SDL_WINDOWEVENT_HIDDEN: SDL_Log("Window %d hidden", event.window.windowID); break;
						case SDL_WINDOWEVENT_EXPOSED: SDL_Log("Window %d exposed", event.window.windowID); break;
						case SDL_WINDOWEVENT_MOVED: SDL_Log("Window %d moved to %d,%d", event.window.windowID, event.window.data1, event.window.data2); break;
						case SDL_WINDOWEVENT_RESIZED: {
							SDL_Log("Window %d resized to %dx%d", event.window.windowID, event.window.data1, event.window.data2);
							renderer.recreate_KHR(deviceManager.get_device_wrapper(), window);
							break;
						}
						case SDL_WINDOWEVENT_SIZE_CHANGED: SDL_Log("Window %d size changed to %dx%d", event.window.windowID, event.window.data1, event.window.data2); break;
						case SDL_WINDOWEVENT_MINIMIZED: {
							SDL_Log("Window %d minimized", event.window.windowID);
							bRendering = false;
							break;
						}
						case SDL_WINDOWEVENT_MAXIMIZED: SDL_Log("Window %d maximized", event.window.windowID); break;
						case SDL_WINDOWEVENT_RESTORED: {
							SDL_Log("Window %d restored", event.window.windowID);
							bRendering = true;
							break;
						}
						case SDL_WINDOWEVENT_ENTER: SDL_Log("Mouse entered window %d", event.window.windowID); break;
						case SDL_WINDOWEVENT_LEAVE: SDL_Log("Mouse left window %d", event.window.windowID); break;
						case SDL_WINDOWEVENT_FOCUS_GAINED: SDL_Log("Window %d gained keyboard focus", event.window.windowID); break;
						case SDL_WINDOWEVENT_FOCUS_LOST: SDL_Log("Window %d lost keyboard focus", event.window.windowID); break;
						case SDL_WINDOWEVENT_CLOSE: SDL_Log("Window %d closed", event.window.windowID); break;
						case SDL_WINDOWEVENT_TAKE_FOCUS: SDL_Log("Window %d is offered a focus", event.window.windowID); break;
						case SDL_WINDOWEVENT_HIT_TEST: SDL_Log("Window %d has a special hit test", event.window.windowID); break;
						case SDL_WINDOWEVENT_DISPLAY_CHANGED: SDL_Log("Window %d moved to %d", event.window.windowID, event.window.data1); break;
						case SDL_WINDOWEVENT_ICCPROF_CHANGED: SDL_Log("ICC profile of window %d on display %d changed", event.window.windowID, event.window.data1); break;
						default: SDL_Log("Window %d got unknown event %d", event.window.windowID, event.window.event); break;
					}
				}
			}
		}

		ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);

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
};