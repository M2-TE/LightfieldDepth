#pragma once

class Window
{
public:
	Window() = default;
	~Window() = default;
	ROF_COPY_MOVE_DELETE(Window)

public:
	void init()
	{
		init_sdl_window();
		create_vulkan_instance();
		create_vulkan_surface();

		ImGui::CreateContext();
		ImGui_ImplSDL2_InitForVulkan(pWindow);


		ImGui::StyleColorsDark();
	}
	void destroy()
	{
		ImGui_ImplSDL2_Shutdown();
		ImGui::DestroyContext();

		SDL_DestroyWindow(pWindow);
		SDL_Quit();

		instance.destroySurfaceKHR(surface);
		DEBUG_ONLY(instance.destroyDebugUtilsMessengerEXT(debugMessenger, nullptr, dld));
		instance.destroy();
	}

	vk::Instance& get_vulkan_instance() { return instance; }
	vk::SurfaceKHR& get_vulkan_surface() { return surface; }
	SDL_Window* get_window() { return pWindow; }

private:
	void init_sdl_window()
	{
		DEBUG_ONLY(VMI_LOG("Debug build.\n"));

		// Create an SDL window that supports Vulkan rendering.
		if (SDL_InitSubSystem(SDL_INIT_VIDEO) != 0) VMI_SDL_ERR();
		pWindow = SDL_CreateWindow(WND_NAME.c_str(), SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WIDTH, HEIGHT, SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE);
		if (pWindow == NULL) VMI_SDL_ERR();
	}
	void create_vulkan_instance()
	{
		// Look for all the available extensions
		std::vector<vk::ExtensionProperties> availableExtensions = vk::enumerateInstanceExtensionProperties();
		if (false) {
			VMI_LOG("Available instance extensions:");
			for (const auto& extension : availableExtensions) VMI_LOG(extension.extensionName);
			VMI_LOG("");
		}

		// Get WSI extensions from SDL
		uint32_t nExtensions;
		if (!SDL_Vulkan_GetInstanceExtensions(pWindow, &nExtensions, NULL)) VMI_SDL_ERR();
		extensions.resize(nExtensions);
		if (!SDL_Vulkan_GetInstanceExtensions(pWindow, &nExtensions, extensions.data())) VMI_SDL_ERR();

		// Debug Logging:
		DEBUG_ONLY(vk::DebugUtilsMessengerCreateInfoEXT messengerInfo = Logging::SetupDebugMessenger(extensions));

		// Log output:
		VMI_LOG("Instance Extensions:");
		std::string spacing = "    ";
		VMI_LOG(spacing << "Required instance extensions:");
		for (const auto& extension : extensions) VMI_LOG(spacing << "- " << extension);
		VMI_LOG("");

		// Create app info
		vk::ApplicationInfo appInfo = vk::ApplicationInfo()
			.setPApplicationName("Blank")
			.setApplicationVersion(VK_MAKE_API_VERSION(0, 0, 1, 0))
			.setPEngineName("Vermillion")
			.setEngineVersion(VK_MAKE_API_VERSION(0, 0, 1, 0))
			.setApiVersion(VK_API_VERSION_1_1);

		// Use validation layer on debug
		DEBUG_ONLY(layers.push_back("VK_LAYER_KHRONOS_validation"));

		// Create instance info
		vk::InstanceCreateInfo createInfo = vk::InstanceCreateInfo()
			.setFlags(vk::InstanceCreateFlags())
			.setPApplicationInfo(&appInfo)
			// Extensions
			.setEnabledExtensionCount(static_cast<uint32_t>(extensions.size()))
			.setPpEnabledExtensionNames(extensions.data())
			// Layers
			.setEnabledLayerCount(static_cast<uint32_t>(layers.size()))
			.setPpEnabledLayerNames(layers.data())
			DEBUG_ONLY(.setPNext(&messengerInfo));

		// Finally, create actual instance
		instance = vk::createInstance(createInfo);

		DEBUG_ONLY(dld.init(instance, vkGetInstanceProcAddr));
		DEBUG_ONLY(debugMessenger = instance.createDebugUtilsMessengerEXT(messengerInfo, nullptr, dld));
	}
	void create_vulkan_surface()
	{
		// Create a Vulkan surface for rendering
		VkSurfaceKHR c_surface;
		if (!SDL_Vulkan_CreateSurface(pWindow, static_cast<VkInstance>(instance), &c_surface)) VMI_SDL_ERR();
		surface = vk::SurfaceKHR(c_surface);
	}

private:
	SDL_Window* pWindow = nullptr;
	vk::Instance instance;
	vk::SurfaceKHR surface;
	DEBUG_ONLY(vk::DispatchLoaderDynamic dld);
	DEBUG_ONLY(vk::DebugUtilsMessengerEXT debugMessenger);

	std::vector<const char*> layers;
	std::vector<const char*> extensions;

	// lazy constants (settings?)
	const std::string WND_NAME = "Vermillion";
	const uint32_t WIDTH = 1280;
	const uint32_t HEIGHT = 720;
};