#pragma once

class DeviceWrapper
{
public:
	DeviceWrapper(vk::PhysicalDevice& physicalDevice, vk::SurfaceKHR& surface) :
		physicalDevice(physicalDevice), iQueue(UINT32_MAX)
	{
		physicalDevice.getProperties(&deviceProperties);
		physicalDevice.getFeatures(&deviceFeatures);
		physicalDevice.getMemoryProperties(&deviceMemProperties);

		query_swapchain_support_details(surface);
		assign_queue_family_index(surface);
	}

	int32_t get_device_score() 
	{
		int32_t deviceScore = 0;

		// Discrete GPUs have a significant performance advantage
		if (deviceProperties.deviceType == vk::PhysicalDeviceType::eDiscreteGpu) deviceScore += 1000;

		// Maximum possible size of textures affects graphics quality
		deviceScore += deviceProperties.limits.maxImageDimension2D;

		if (iQueue == UINT32_MAX) return -1; // check for valid queue index
		else if (formats.empty() || presentModes.empty()) return -1;
		else return deviceScore;
	}
	void create_logical_device()
	{
		float qPriority = 1.0f;
		vk::DeviceQueueCreateInfo queueCreateInfo = vk::DeviceQueueCreateInfo()
			.setQueueFamilyIndex(iQueue)
			.setQueueCount(1)
			.setPQueuePriorities(&qPriority);

		vk::PhysicalDeviceFeatures deviceFeatures = vk::PhysicalDeviceFeatures();
		// TODO: set specific features here

		if (false) {
			VMI_LOG("Available device extensions:");
			std::vector<vk::ExtensionProperties> availableExtensions = physicalDevice.enumerateDeviceExtensionProperties();
			for (const auto& extension : availableExtensions) VMI_LOG(extension.extensionName);
			VMI_LOG("");
		}

		VMI_LOG("Required device extensions:");
		std::vector<const char*> requiredDeviceExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
		for (const auto& extension : requiredDeviceExtensions) VMI_LOG(extension);
		VMI_LOG("");


		vk::DeviceCreateInfo createInfo = vk::DeviceCreateInfo()
			// queues
			.setPQueueCreateInfos(&queueCreateInfo)
			.setQueueCreateInfoCount(1)
			// extensions
			.setPpEnabledExtensionNames(requiredDeviceExtensions.data())
			.setEnabledExtensionCount(static_cast<uint32_t>(requiredDeviceExtensions.size()))
			// device features
			.setPEnabledFeatures(&deviceFeatures);

		// Create logical device
		logicalDevice = physicalDevice.createDevice(createInfo);

		// get actual handle for graphics queue
		queue = logicalDevice.getQueue(iQueue, 0u);
	}
	void destroy_logical_device() { logicalDevice.destroy(); }

private:
	void assign_queue_family_index(vk::SurfaceKHR& surface)
	{
		// find a queue family that supports both graphics and presentation
		std::vector<vk::QueueFamilyProperties> queueFamilies = physicalDevice.getQueueFamilyProperties();
		for (int i = 0; i < queueFamilies.size(); i++) {

#ifdef _DEBUG
			VMI_LOG("Queue family capabilities at index " << i);
			if(queueFamilies[i].queueFlags & vk::QueueFlagBits::eGraphics) VMI_LOG("\t-> graphics");
			if(queueFamilies[i].queueFlags & vk::QueueFlagBits::eTransfer) VMI_LOG("\t-> transfer");
			if(queueFamilies[i].queueFlags & vk::QueueFlagBits::eCompute) VMI_LOG("\t-> compute");
			if(queueFamilies[i].queueFlags & vk::QueueFlagBits::eProtected) VMI_LOG("\t-> protected");
			if(queueFamilies[i].queueFlags & vk::QueueFlagBits::eSparseBinding) VMI_LOG("\t-> sparse binding");
			VMI_LOG("");
#endif

			if (queueFamilies[i].queueFlags & vk::QueueFlagBits::eGraphics &&
				physicalDevice.getSurfaceSupportKHR(i, surface)) {

				iQueue = i;
				break;
			}
		}
	}
	void query_swapchain_support_details(vk::SurfaceKHR& surface)
	{
		capabilities = physicalDevice.getSurfaceCapabilitiesKHR(surface);
		formats = physicalDevice.getSurfaceFormatsKHR(surface);
		presentModes = physicalDevice.getSurfacePresentModesKHR(surface);
	}

public:
	vk::PhysicalDevice physicalDevice;
	vk::Device logicalDevice;

	vk::Queue queue;
	uint32_t iQueue;

	// some properties of the device
	vk::SurfaceCapabilitiesKHR capabilities;
	std::vector<vk::SurfaceFormatKHR> formats;
	std::vector<vk::PresentModeKHR> presentModes;

	vk::PhysicalDeviceProperties deviceProperties;
	vk::PhysicalDeviceFeatures deviceFeatures;
	vk::PhysicalDeviceMemoryProperties deviceMemProperties;
};