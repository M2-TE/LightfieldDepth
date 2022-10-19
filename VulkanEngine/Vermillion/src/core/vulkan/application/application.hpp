#pragma once

#include "input.hpp"
#include "scene_objects/scene.hpp"
#include "window.hpp"
#include "devices/device_manager.hpp"
#include "renderer.hpp"

class Application
{
public:
	Application()
	{
		VMI_LOG("[Initializing] Independent vulkan functions...");
		vk::DynamicLoader dl;
		PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr = dl.getProcAddress<PFN_vkGetInstanceProcAddr>("vkGetInstanceProcAddr");
		VULKAN_HPP_DEFAULT_DISPATCHER.init(vkGetInstanceProcAddr);

		window.init(fullscreenMode ? fullscreenResolution : windowedResolution, fullscreenMode);
		deviceManager.init(window.get_vulkan_instance(), window.get_vulkan_surface());
		renderer.init(deviceManager.get_device_wrapper(), window, std::string("lightfields/").append(mainFolder).append(subFolder).c_str());
		scene.init();
		VMI_LOG("[Initialization Complete]" << std::endl);
	}
	~Application()
	{
		deviceManager.get_logical_device().waitIdle();

		scene.destroy();
		renderer.destroy(deviceManager.get_device_wrapper(), scene.reg);

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
		imgui_begin();
		if (!poll_inputs()) return false;
		handle_inputs();
		scene.update();
		renderer.handle_allocations(deviceManager.get_device_wrapper(), scene.reg);
		imgui_end();

		if (!bPaused) render();
		else stall();

		return true;
	}
	void render()
	{
		renderer.render(deviceManager.get_device_wrapper(), scene.reg, iRenderMode);
	}
	void stall()
	{
		ImGui::EndFrame(); // manually end imgui frame
		SDL_Delay(200); // slow down update loop while no rendering occurs
	}

	void imgui_begin()
	{
		ImGui_ImplVulkan_NewFrame();
		ImGui_ImplSDL2_NewFrame();
		ImGui::NewFrame();
	}
	void imgui_end()
	{
		switch (iRenderMode) {
			case 0: ImGui::Text("Current render mode: Color - RGB"); break;
			case 1: ImGui::Text("Current render mode: Gradients Horizontal - RG"); break;
			case 2: ImGui::Text("Current render mode: Gradients Vertical - RG"); break;
			case 3: ImGui::Text("Current render mode: Disparity - Heatmap RGB"); break;
			case 4: ImGui::Text("Current render mode: Depth - Heatmap RGB"); break;
		}
		ImGui::Text("%.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);

		// TODO: load all folder names in "lightfields" dynamically
		const char* mainFolderPaths[] = { "additional", "stratified", "test", "training" };
		bool bMain = ImGui::ListBox("Main Folder", &iMainFolder, mainFolderPaths, std::size(mainFolderPaths), 2);

		// TODO: load all folder names within current mainFolder and offer them as options
		const char* subFolderPaths[] = { "cotton", "boxes"};
		bool bSub = ImGui::ListBox("Sub Folder", &iSubFolder, subFolderPaths, std::size(subFolderPaths), 2);

		if (bMain || bSub) {
			mainFolder.assign(mainFolderPaths[iMainFolder]).append("/");
			subFolder.assign(subFolderPaths[iSubFolder]).append("/");
			resize(true);
		}

		ImGui::EndFrame();
	}
	bool poll_inputs()
	{
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
						case SDL_WINDOWEVENT_FOCUS_LOST: bPaused = true; break;

						case SDL_WINDOWEVENT_FOCUS_GAINED: if (fullscreenMode == SDL_WINDOW_FULLSCREEN) break;
						case SDL_WINDOWEVENT_RESTORED: bPaused = false; break;
					}
				}
			}
		}
		return true;
	}
	void handle_inputs()
	{
		renderer.handle_input(input);

		if (input.keysPressed.count(SDLK_F11)) {
			toggle_fullscreen();
		}
		if (input.keysPressed.count(SDLK_F10)) {
			renderer.dump_mem_vma();
		}
		if (input.keysPressed.count(SDLK_F1)) iRenderMode = 0;
		else if (input.keysPressed.count(SDLK_F2)) iRenderMode = 1;
		else if (input.keysPressed.count(SDLK_F3)) iRenderMode = 2;
		else if (input.keysPressed.count(SDLK_F4)) iRenderMode = 3;
		else if (input.keysPressed.count(SDLK_F5)) iRenderMode = 4;
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
	void resize(bool bForceRebuild = false) // resize swapchain using actual window size TODO: DISABLE BOOL?
	{
		Sint32 w, h;
		SDL_GetWindowSize(window.get_window(), &w, &h);
		VMI_LOG("Attempting swapchain rebuild: (" << bForceRebuild << ") " << w << "x" << h);
		renderer.recreate_KHR(deviceManager.get_device_wrapper(), window, bForceRebuild, std::string("lightfields/").append(mainFolder).append(subFolder).c_str());
	}

private:
	Window window;
	DeviceManager deviceManager;
	Renderer renderer;
	Input input;
	Scene scene;
	uint32_t iRenderMode = 0;

	// lightfield data directory
	int iMainFolder = 0, iSubFolder = 0;
	std::string mainFolder = "training/";
	std::string subFolder = "cotton/";

	bool bPaused = false;
	uint32_t fullscreenMode = 0;
	//const uint32_t fullscreenModeTarget = SDL_WINDOW_FULLSCREEN;
	const uint32_t fullscreenModeTarget = SDL_WINDOW_FULLSCREEN_DESKTOP;
	std::pair<Sint32, Sint32> fullscreenResolution = { 1920, 1080 };
	std::pair<Sint32, Sint32> windowedResolution = { 512, 512 };
};