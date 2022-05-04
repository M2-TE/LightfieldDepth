#pragma once

#include "device_manager.hpp"

class Application
{
public:
	Application() = default;
	~Application() = default;
	ROF_COPY_MOVE_DELETE(Application)

public:
	void run()
	{
		init();
	}

private:
	void init()
	{
		init_sdl_window();
		create_vulkan_instance();
		create_vulkan_surface();

		deviceManager.init(instance, surface);
		deviceManager.create_logical_device(device);

		//CreateSwapChain();
		//CreateImageViews();
		//CreateRenderPass();
		//CreateGraphicsPipeline();
		//CreateFramebuffers();
		//CreateCommandPool();
		//CreateCommandBuffers();
		//CreateSyncObjects();
	}

	void init_sdl_window()
	{
		DEBUG_ONLY(VMI_LOG("Debug build.\n"));

		// Create an SDL window that supports Vulkan rendering.
		if (SDL_InitSubSystem(SDL_INIT_VIDEO) != 0) VMI_SDL_ERR();
		pWindow = SDL_CreateWindow(WND_NAME.c_str(), SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WIDTH, HEIGHT, SDL_WINDOW_VULKAN);
		if (pWindow == NULL) VMI_SDL_ERR();
	}
	void create_vulkan_instance()
	{
		// Look for all the available extensions
		std::vector<vk::ExtensionProperties> availableExtensions = vk::enumerateInstanceExtensionProperties();
		VMI_LOG("Available extensions:");
		for (const auto& extension : availableExtensions) VMI_LOG(extension.extensionName);
		VMI_LOG("");

		// Get WSI extensions from SDL
		uint32_t nExtensions;
		if (!SDL_Vulkan_GetInstanceExtensions(pWindow, &nExtensions, NULL)) VMI_SDL_ERR();
		extensions.resize(nExtensions);
		if (!SDL_Vulkan_GetInstanceExtensions(pWindow, &nExtensions, extensions.data())) VMI_SDL_ERR();

		// Debug Logging:
		DEBUG_ONLY(vk::DebugUtilsMessengerCreateInfoEXT messengerInfo = Logging::SetupDebugMessenger(extensions));

		// Log output:
		std::cout << "Required extensions:\n";
		for (const auto& extension : extensions) std::cout << extension << '\n';
		std::cout << std::endl;

		// Create app info
		vk::ApplicationInfo appInfo = vk::ApplicationInfo()
			.setPApplicationName("Blank Game")
			.setApplicationVersion(VK_MAKE_API_VERSION(0, 0, 1, 0))
			.setPEngineName("Vermillion")
			.setEngineVersion(VK_MAKE_API_VERSION(0, 0, 1, 0))
			.setApiVersion(VK_API_VERSION_1_0);

		// Use validation layer on debug
		DEBUG_ONLY(requiredLayers.push_back("VK_LAYER_KHRONOS_validation"));

		// Create instance info
		vk::InstanceCreateInfo createInfo = vk::InstanceCreateInfo()
			.setFlags(vk::InstanceCreateFlags())
			.setPApplicationInfo(&appInfo)
			// Extensions
			.setEnabledExtensionCount(static_cast<uint32_t>(extensions.size()))
			.setPpEnabledExtensionNames(extensions.data())
			// Layers
			.setEnabledLayerCount(static_cast<uint32_t>(requiredLayers.size()))
			.setPpEnabledLayerNames(requiredLayers.data())
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
	DeviceManager deviceManager;

	// lazy constants (settings?)
	const std::string WND_NAME = "Vermillion";
	const uint32_t WIDTH = 1280;
	const uint32_t HEIGHT = 720;
	const int MAX_FRAMES_IN_FLIGHT = 2;

	SDL_Window* pWindow = nullptr;
	vk::Instance instance;
	vk::SurfaceKHR surface;
	vk::Device device;
	vk::Queue qGraphics, qPresent;
	vk::SwapchainKHR swapchain;
	std::vector<vk::Image> swapchainImages;
	std::vector<vk::ImageView> swapchainImageViews;
	std::vector<vk::Framebuffer> swapchainFramebuffers;
	vk::Format swapchainImageFormat;
	vk::Extent2D swapchainExtent;
	vk::ShaderModule vertShaderModule;
	vk::ShaderModule fragShaderModule;
	vk::RenderPass renderPass;
	vk::PipelineLayout pipelineLayout;
	vk::Pipeline graphicsPipeline;
	vk::CommandPool commandPool;
	std::vector<vk::CommandBuffer> commandBuffers;

	std::vector<vk::Semaphore> imageAvailableSemaphores;
	std::vector<vk::Semaphore> renderFinishedSemaphores;
	std::vector<vk::Fence> inFlightFences;
	std::vector<vk::Fence> imagesInFlight;
	size_t currentFrame = 0;

	std::vector<const char*> requiredLayers;
	std::vector<const char*> extensions;
	DEBUG_ONLY(vk::DispatchLoaderDynamic dld);
	DEBUG_ONLY(vk::DebugUtilsMessengerEXT debugMessenger);
};