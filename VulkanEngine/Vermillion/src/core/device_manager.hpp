#pragma once

#include "device_wrapper.hpp"

class DeviceManager
{
public:
	DeviceManager() = default;
	~DeviceManager() = default;
	ROF_COPY_MOVE_DELETE(DeviceManager)

public:
	void init(vk::Instance& instance, const vk::SurfaceKHR& surface)
	{
		std::vector<vk::PhysicalDevice> physicalDevices = instance.enumeratePhysicalDevices();
		if (physicalDevices.empty()) VMI_ERR("Failed to find GPUs with Vulkan support.");
		devices.reserve(physicalDevices.size());
		for (const vk::PhysicalDevice& physicalDevice : physicalDevices) devices.emplace_back(physicalDevice, surface);

		pick_best_physical_device(surface);
	}
	void destroy()
	{
		logicalDevice.destroy();
	}
	void create_logical_device()
	{
		DeviceWrapper& deviceWrapper = devices[iCurrentDevice];

		std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos;
		std::set<uint32_t> uniqueQFamilies = { 
			deviceWrapper.indices.iGraphicsFamily.value(), 
			deviceWrapper.indices.iPresentFamily.value() 
		};

		float qPriority = 1.0f;
		for (uint32_t queueFamily : uniqueQFamilies) {
			vk::DeviceQueueCreateInfo qCreateInfo = vk::DeviceQueueCreateInfo()
				.setQueueFamilyIndex(queueFamily)
				.setQueueCount(1)
				.setPQueuePriorities(&qPriority);
			queueCreateInfos.push_back(qCreateInfo);
		}

		vk::PhysicalDeviceFeatures deviceFeatures = vk::PhysicalDeviceFeatures();
		// TODO: set specific features here

		std::vector<const char*> requiredDeviceExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
		vk::DeviceCreateInfo createInfo = vk::DeviceCreateInfo()
			// queues
			.setPQueueCreateInfos(queueCreateInfos.data())
			.setQueueCreateInfoCount(static_cast<uint32_t>(queueCreateInfos.size()))
			// extensions
			.setPpEnabledExtensionNames(requiredDeviceExtensions.data())
			.setEnabledExtensionCount(static_cast<uint32_t>(requiredDeviceExtensions.size()))
			// device features
			.setPEnabledFeatures(&deviceFeatures);

		// Create logical device
		logicalDevice = deviceWrapper.physicalDevice.createDevice(createInfo);
	}

	inline vk::Device& get_logical_device() { return logicalDevice; }
	inline DeviceWrapper& get_device_wrapper() { return devices[iCurrentDevice]; }

private:
	void pick_best_physical_device(const vk::SurfaceKHR& surface)
	{
		int highscore = -1;
		for (int i = 0; i < devices.size(); i++)
		{
			int score = get_device_score(devices[i], surface);
			if (score > highscore) {
				highscore = score;
				iCurrentDevice = i;
			}
		}

		if (highscore == -1) VMI_ERR("Failed to find a suitable GPU.");
	}
	int get_device_score(const DeviceWrapper& device, const vk::SurfaceKHR& surface)
	{
		int score = 0;

		// Discrete GPUs have a significant performance advantage
		if (device.deviceProperties.deviceType == vk::PhysicalDeviceType::eDiscreteGpu) score += 1000;

		// Maximum possible size of textures affects graphics quality
		score += device.deviceProperties.limits.maxImageDimension2D;

		// Application can't function without these features
		if (!device.deviceFeatures.geometryShader) score = -1;
		else if (!device.indices.iGraphicsFamily.has_value() || !device.indices.iPresentFamily.has_value()) score = -1;
		else if (!are_extensions_supported(device.physicalDevice)) score = -1;
		else if (device.swapchain.formats.empty() || device.swapchain.presentModes.empty()) score = -1;

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

private:
	vk::Device logicalDevice;
	std::vector<DeviceWrapper> devices;
	int iCurrentDevice = -1;
};