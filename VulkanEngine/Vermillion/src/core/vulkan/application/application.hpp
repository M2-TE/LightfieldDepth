#pragma once

#include "input.hpp"
#include "scene_objects/scene.hpp"
#include "window.hpp"
#include "devices/device_manager.hpp"
#include "renderer.hpp"
#include "utils/file_utils.hpp"

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
		renderer.render(deviceManager.get_device_wrapper(), scene.reg, pushConstant);
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
		ImGui::Begin("Render Info");
		ImGui::Text("%.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
		switch (pushConstant.iRenderMode) {
			case 0: ImGui::Text("Color - RGB"); break;
			case 1: ImGui::Text("Gradients Horizontal - RG"); break;
			case 2: ImGui::Text("Gradients Vertical - RG"); break;
			case 3: ImGui::Text("Disparity - Heatmap RGB"); break;
			case 4: ImGui::Text("Depth - Heatmap RGB"); break;
			case 5: {
				ImGui::Text("0-filler spots - RGB");
				ImGui::Text("Color refers to filter size used to fill:");
				ImGui::Text("Red = 5");
				ImGui::Text("Green = 7");
				ImGui::Text("Blue = 9");
				ImGui::Text("White = N/A");
				break;
			}
		}

		// TODO: show current iFilterMode status
		//if (pushConstant.bGradientFillers) ImGui::Text("0-filler enabled");
		//else ImGui::Text("0-filler disabled");

		ImGui::Text("\nKeybinds:");
		ImGui::Text("F1 - F6: render modes");
		ImGui::Text("LCTRL: toggle 0.0f filler");
		ImGui::Text("F10: device memory dump");
		ImGui::Text("F11: fullscreen");
		ImGui::End();

		ImGui::Begin("Depth Sliders");
		ImGui::Text("Depth = 1 / A + B * disparity");
		ImGui::SliderFloat("Depth Mod A", &pushConstant.depthModA, 0.0f, 1.0f);
		ImGui::SliderFloat("Depth Mod B", &pushConstant.depthModB, 0.0f, 4.0f);
		ImGui::End();

		ImGui::Begin("Source Selection");
		std::string currentDir = std::filesystem::current_path().append("lightfields").string();

		// load all folder names in "lightfields" dynamically
		std::vector<std::string> mainDirs = get_directories("lightfields");
		iMainFolder = std::clamp(iMainFolder, 0, (int)mainDirs.size());
		ImGui::Text("Main Folder");
		bool bMain = ImGui::ListBox("##0", &iMainFolder, VectorOfStringGetter, (void*)&mainDirs, (int)mainDirs.size());

		// load all folder names within current mainFolder and offer them as options
		std::vector<std::string> subDirs = get_directories(std::filesystem::path("lightfields").append(mainDirs[iMainFolder]).string());
		iSubFolder = std::clamp(iSubFolder, 0, (int)subDirs.size());
		ImGui::Text("Sub Folder");
		bool bSub = ImGui::ListBox("##1", &iSubFolder, VectorOfStringGetter, (void*)&subDirs, (int)subDirs.size());

		if (bMain || bSub) {
			mainFolder.assign(mainDirs[iMainFolder]).append("/");
			subFolder.assign(subDirs[iSubFolder]).append("/");
			resize(true);
		}
		ImGui::End();

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
		renderer.handle_input(deviceManager.get_device_wrapper(), input);

		if (input.keysPressed.count(SDLK_F11)) {
			toggle_fullscreen();
		}
		if (input.keysPressed.count(SDLK_F10)) {
			renderer.dump_mem_vma();
		}
		
		// shift key to switch between render mode and filter mode
		if (input.keysDown.count(SDLK_LSHIFT) || input.keysDown.count(SDLK_RSHIFT)) {
			if (input.keysPressed.count(SDLK_F1)) pushConstant.iFilterMode = 0;
			else if (input.keysPressed.count(SDLK_F2)) pushConstant.iFilterMode = 1;
			else if (input.keysPressed.count(SDLK_F3)) pushConstant.iFilterMode = 2;
			else if (input.keysPressed.count(SDLK_F4)) pushConstant.iFilterMode = 3;
			else if (input.keysPressed.count(SDLK_F5)) pushConstant.iFilterMode = 4;
		}
		else {
			if (input.keysPressed.count(SDLK_F1)) pushConstant.iRenderMode = 0;
			else if (input.keysPressed.count(SDLK_F2)) pushConstant.iRenderMode = 1;
			else if (input.keysPressed.count(SDLK_F3)) pushConstant.iRenderMode = 2;
			else if (input.keysPressed.count(SDLK_F4)) pushConstant.iRenderMode = 3;
			else if (input.keysPressed.count(SDLK_F5)) pushConstant.iRenderMode = 4;
			else if (input.keysPressed.count(SDLK_F6)) pushConstant.iRenderMode = 5;
			else if (input.keysPressed.count(SDLK_F7)) pushConstant.iRenderMode = 6;
		}
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
		VMI_LOG("Attempting swapchain rebuild: " << w << "x" << h);
		renderer.recreate_KHR(deviceManager.get_device_wrapper(), window, bForceRebuild, std::string("lightfields/").append(mainFolder).append(subFolder).c_str());
	}

private:
	Window window;
	DeviceManager deviceManager;
	Renderer renderer;
	Input input;
	Scene scene;
	PC pushConstant;

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