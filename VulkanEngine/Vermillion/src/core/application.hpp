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
		if (fullscreenMode) window.init(fullscreenResolution, fullscreenMode);
		else window.init(windowedResolution, fullscreenMode);

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

		input.flush();
		SDL_Event sdlEevent;
		while (SDL_PollEvent(&sdlEevent)) {

			ImGui_ImplSDL2_ProcessEvent(&sdlEevent);

			switch (sdlEevent.type) {
				case SDL_QUIT: return false;

				case SDL_MOUSEBUTTONDOWN:
				case SDL_MOUSEBUTTONUP: input.register_mouse_button_event(sdlEevent.button); break;
				case SDL_MOUSEMOTION: input.register_mouse_motion_event(sdlEevent.motion); break;

				case SDL_KEYUP:
				case SDL_KEYDOWN: input.register_keyboard_event(sdlEevent.key); break;

				case SDL_WINDOWEVENT: {
					//print_event(&event);
					switch (sdlEevent.window.event) {
						//case SDL_WINDOWEVENT_SIZE_CHANGED:
						case SDL_WINDOWEVENT_RESIZED: resize(true); break;

						case SDL_WINDOWEVENT_MINIMIZED:
						case SDL_WINDOWEVENT_FOCUS_LOST: bPaused = false; break;

						case SDL_WINDOWEVENT_FOCUS_GAINED: if (fullscreenMode == SDL_WINDOW_FULLSCREEN) break;
						case SDL_WINDOWEVENT_RESTORED: bPaused = true; break;
					}
				}
			}
		}

		// TODO: move this to a higher-level class?
		if (input.keysPressed.count(SDLK_F11)) {
			toggle_fullscreen();
		}
		if (input.keysPressed.count(SDLK_F10)) {
			renderer.dump_mem_vma();
		}

		ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);

		if (bPaused) render();
		else stall();

		return true;
	}

	void print_event(SDL_Event* event)
	{
		if (event->type == SDL_WINDOWEVENT) {
			switch (event->window.event) {
				case SDL_WINDOWEVENT_SHOWN:
					SDL_Log("Window %d shown", event->window.windowID);
					break;
				case SDL_WINDOWEVENT_HIDDEN:
					SDL_Log("Window %d hidden", event->window.windowID);
					break;
				case SDL_WINDOWEVENT_EXPOSED:
					SDL_Log("Window %d exposed", event->window.windowID);
					break;
				case SDL_WINDOWEVENT_MOVED:
					SDL_Log("Window %d moved to %d,%d",
						event->window.windowID, event->window.data1,
						event->window.data2);
					break;
				case SDL_WINDOWEVENT_RESIZED:
					SDL_Log("Window %d resized to %dx%d",
						event->window.windowID, event->window.data1,
						event->window.data2);
					break;
				case SDL_WINDOWEVENT_SIZE_CHANGED:
					SDL_Log("Window %d size changed to %dx%d",
						event->window.windowID, event->window.data1,
						event->window.data2);
					break;
				case SDL_WINDOWEVENT_MINIMIZED:
					SDL_Log("Window %d minimized", event->window.windowID);
					break;
				case SDL_WINDOWEVENT_MAXIMIZED:
					SDL_Log("Window %d maximized", event->window.windowID);
					break;
				case SDL_WINDOWEVENT_RESTORED:
					SDL_Log("Window %d restored", event->window.windowID);
					break;
				case SDL_WINDOWEVENT_ENTER:
					SDL_Log("Mouse entered window %d",
						event->window.windowID);
					break;
				case SDL_WINDOWEVENT_LEAVE:
					SDL_Log("Mouse left window %d", event->window.windowID);
					break;
				case SDL_WINDOWEVENT_FOCUS_GAINED:
					SDL_Log("Window %d gained keyboard focus",
						event->window.windowID);
					break;
				case SDL_WINDOWEVENT_FOCUS_LOST:
					SDL_Log("Window %d lost keyboard focus",
						event->window.windowID);
					break;
				case SDL_WINDOWEVENT_CLOSE:
					SDL_Log("Window %d closed", event->window.windowID);
					break;
#if SDL_VERSION_ATLEAST(2, 0, 5)
				case SDL_WINDOWEVENT_TAKE_FOCUS:
					SDL_Log("Window %d is offered a focus", event->window.windowID);
					break;
				case SDL_WINDOWEVENT_HIT_TEST:
					SDL_Log("Window %d has a special hit test", event->window.windowID);
					break;
#endif
				default:
					SDL_Log("Window %d got unknown event %d",
						event->window.windowID, event->window.event);
					break;
			}
		}
	}
	void resize(bool bForceRebuild = false) // resize swapchain using actual window size TODO: DISABLE BOOL?
	{
		Sint32 w, h;
		SDL_GetWindowSize(window.get_window(), &w, &h);
		VMI_LOG("Attempting swapchain rebuild: (" << bForceRebuild << ") " << w << "x" << h);
		renderer.recreate_KHR(deviceManager.get_device_wrapper(), window, bForceRebuild);
	}
	void toggle_fullscreen()
	{
		if (fullscreenMode) {
			SDL_SetWindowFullscreen(window.get_window(), 0);
			fullscreenMode = 0;
		}
		else {
			SDL_SetWindowFullscreen(window.get_window(), fullscreenModeTarget);
			fullscreenMode = fullscreenModeTarget;
		}

		resize(true);
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

	bool bPaused = true;
	uint32_t fullscreenMode = 0;
	//const uint32_t fullscreenModeTarget = SDL_WINDOW_FULLSCREEN;
	const uint32_t fullscreenModeTarget = SDL_WINDOW_FULLSCREEN_DESKTOP;
	std::pair<Sint32, Sint32> fullscreenResolution = { 1920, 1080 };
	std::pair<Sint32, Sint32> windowedResolution = { 1280, 720 };
};