#pragma once

class DeviceWrapper
{
public:
	DeviceWrapper(const vk::PhysicalDevice& physicalDevice, const vk::SurfaceKHR& surface) :
		physicalDevice(physicalDevice),
		indices(physicalDevice, surface),
		swapchain(physicalDevice, surface)
	{
		physicalDevice.getProperties(&deviceProperties);
		physicalDevice.getFeatures(&deviceFeatures);
		physicalDevice.getMemoryProperties(&deviceMemProperties);
	}

	vk::PhysicalDevice physicalDevice;
	vk::PhysicalDeviceProperties deviceProperties;
	vk::PhysicalDeviceFeatures deviceFeatures;
	vk::PhysicalDeviceMemoryProperties deviceMemProperties;

	vk::Queue graphicsQueue;
	vk::Queue presentQueue;

	struct QueueFamilyIndices
	{
		QueueFamilyIndices(const vk::PhysicalDevice& physicalDevice, const vk::SurfaceKHR& surface)
		{
			std::vector<vk::QueueFamilyProperties> queueFamilies = physicalDevice.getQueueFamilyProperties();
			for (int i = 0; i < queueFamilies.size(); i++) {

				if (queueFamilies[i].queueFlags & vk::QueueFlagBits::eGraphics) {
					iGraphicsFamily = i;

					// check if graphics family has the same index as the present family
					if (physicalDevice.getSurfaceSupportKHR(i, surface)) {
						iPresentFamily = i;
						break;
					}
				}
			}

			if (!iPresentFamily.has_value()) {
				VMI_LOG("Warning: Indices for graphics and present queue differ.");
				for (int i = 0; i < queueFamilies.size(); i++) {

					// just get any present index, if  present and graphics are not the same queue
					if (physicalDevice.getSurfaceSupportKHR(i, surface)) {
						iPresentFamily = i;
						break;
					}
				}
			}
		}
		std::optional<uint32_t> iGraphicsFamily;
		std::optional<uint32_t> iPresentFamily;
	} indices;

	struct SwapchainSupportDetails {
		SwapchainSupportDetails(const vk::PhysicalDevice& physicalDevice, const vk::SurfaceKHR& surface)
		{
			capabilities = physicalDevice.getSurfaceCapabilitiesKHR(surface);
			formats = physicalDevice.getSurfaceFormatsKHR(surface);
			presentModes = physicalDevice.getSurfacePresentModesKHR(surface);

		}
		vk::SurfaceCapabilitiesKHR capabilities;
		std::vector<vk::SurfaceFormatKHR> formats;
		std::vector<vk::PresentModeKHR> presentModes;
	} swapchain;
};