#pragma once

class DeviceManager
{
public:
	DeviceManager() = default;
	~DeviceManager() = default;
	ROF_COPY_MOVE_DELETE(DeviceManager)

public:
	void update(vk::Instance instance, const vk::SurfaceKHR& surface)
	{
		physicalDevices = instance.enumeratePhysicalDevices();
		if (physicalDevices.empty()) VMI_ERR("Failed to find GPUs with Vulkan support.");

		pick_best_physical_device(surface);
	}
	vk::PhysicalDevice& get_device()
	{
		return physicalDevices[iCurrentDevice];
	}

private:

	void pick_best_physical_device(const vk::SurfaceKHR& surface)
	{
		int highscore = -1;
		for (int i = 0; i < physicalDevices.size(); i++)
		{
			int score = get_device_score(physicalDevices[i], surface);
			if (score > highscore) {
				highscore = score;
				iCurrentDevice = i;
			}
		}

		if (highscore == -1) VMI_ERR("Failed to find a suitable GPU.");
	}
	int get_device_score(const vk::PhysicalDevice& device, const vk::SurfaceKHR& surface)
	{
		vk::PhysicalDeviceProperties deviceProperties;
		vk::PhysicalDeviceFeatures deviceFeatures;
		vk::PhysicalDeviceMemoryProperties deviceMemProperties;
		device.getProperties(&deviceProperties);
		device.getFeatures(&deviceFeatures);
		device.getMemoryProperties(&deviceMemProperties);

		int score = 0;

		// Discrete GPUs have a significant performance advantage
		if (deviceProperties.deviceType == vk::PhysicalDeviceType::eDiscreteGpu) score += 1000;

		// Maximum possible size of textures affects graphics quality
		score += deviceProperties.limits.maxImageDimension2D;

		// Application can't function without geometry shaders
		if (!deviceFeatures.geometryShader) score = -1;
		else if (!are_queue_families_supported(device, surface)) score = -1;
		else if (!are_extensions_supported(device)) score = -1;
		else if (!are_swapchain_features_supported(device, surface)) score = -1;

		return score;
	}
	bool are_extensions_supported(const vk::PhysicalDevice& device)
	{
		// required extensions which need to be present
		std::vector<const char*> requiredDeviceExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
		std::set<std::string> requiredExtensions(requiredDeviceExtensions.begin(), requiredDeviceExtensions.end());

		// query available extensions in device directly
		std::vector<vk::ExtensionProperties> availableExtensions = device.enumerateDeviceExtensionProperties();

		// determine which requested extensions are available in device
		for (const auto& extension : availableExtensions) {
			requiredExtensions.erase(extension.extensionName);
		}

		// when all required extensions are present, should be empty
		return requiredExtensions.empty();
	}
	bool are_queue_families_supported(const vk::PhysicalDevice& device, const vk::SurfaceKHR& surface)
	{
		// TODO: could check if queues are the same queue index for performance

		struct QueueFamilyIndices
		{
			bool IsComplete()
			{
				return
					graphicsFamily.has_value() &&
					presentFamily.has_value();
			}

			std::optional<uint32_t> graphicsFamily;
			std::optional<uint32_t> presentFamily;
		};

		QueueFamilyIndices indices;
		std::vector<vk::QueueFamilyProperties> queueFamilies = device.getQueueFamilyProperties();
		for (int i = 0; i < queueFamilies.size(); i++) {
			if (queueFamilies[i].queueFlags & vk::QueueFlagBits::eGraphics) {
				indices.graphicsFamily = i;
				if (indices.IsComplete()) break;
			}
			if (device.getSurfaceSupportKHR(i, surface)) {
				indices.presentFamily = i;
				if (indices.IsComplete()) break;
			}
		}
		return indices.IsComplete();
	}
	bool are_swapchain_features_supported(const vk::PhysicalDevice& device, const vk::SurfaceKHR& surface) {

		struct SwapchainSupportDetails {
			vk::SurfaceCapabilitiesKHR capabilities;
			std::vector<vk::SurfaceFormatKHR> formats;
			std::vector<vk::PresentModeKHR> presentModes;
		};

		SwapchainSupportDetails details;
		details.capabilities = device.getSurfaceCapabilitiesKHR(surface);
		details.formats = device.getSurfaceFormatsKHR(surface);
		details.presentModes = device.getSurfacePresentModesKHR(surface);

		return !(details.formats.empty() || details.presentModes.empty());
	}


private:
	std::vector<vk::PhysicalDevice> physicalDevices;
	int iCurrentDevice = -1;
};