#pragma once

class ImguiWrapper
{
public:
	ImguiWrapper() = default;
	~ImguiWrapper() = default;
	ROF_COPY_MOVE_DELETE(ImguiWrapper)

public:
	void init(DeviceWrapper& deviceWrapper, RingBuffer& ringBuffer, Window& window, vk::RenderPass& renderPass)
	{
		imgui_create_desc_pool(deviceWrapper);
		imgui_init_vulkan(deviceWrapper, ringBuffer, window, renderPass);
		imgui_upload_fonts(deviceWrapper, ringBuffer);
	}
	void destroy(DeviceWrapper& deviceWrapper)
	{
		deviceWrapper.logicalDevice.destroyDescriptorPool(descPool);
	}

private:
	void imgui_create_desc_pool(DeviceWrapper& deviceWrapper)
	{
		uint32_t descCountImgui = 1000;
		std::array<vk::DescriptorPoolSize, 11> poolSizes =
		{
			vk::DescriptorPoolSize(vk::DescriptorType::eSampler, descCountImgui),
			vk::DescriptorPoolSize(vk::DescriptorType::eCombinedImageSampler, descCountImgui),
			vk::DescriptorPoolSize(vk::DescriptorType::eSampledImage, descCountImgui),
			vk::DescriptorPoolSize(vk::DescriptorType::eStorageImage, descCountImgui),
			vk::DescriptorPoolSize(vk::DescriptorType::eUniformTexelBuffer, descCountImgui),
			vk::DescriptorPoolSize(vk::DescriptorType::eStorageTexelBuffer, descCountImgui),
			vk::DescriptorPoolSize(vk::DescriptorType::eUniformBuffer, descCountImgui),
			vk::DescriptorPoolSize(vk::DescriptorType::eStorageBuffer, descCountImgui),
			vk::DescriptorPoolSize(vk::DescriptorType::eUniformBufferDynamic, descCountImgui),
			vk::DescriptorPoolSize(vk::DescriptorType::eStorageBufferDynamic, descCountImgui),
			vk::DescriptorPoolSize(vk::DescriptorType::eInputAttachment, descCountImgui)
		};

		vk::DescriptorPoolCreateInfo info = vk::DescriptorPoolCreateInfo()
			.setFlags(vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet)
			.setMaxSets(descCountImgui * (uint32_t)poolSizes.size())
			.setPoolSizeCount((uint32_t)poolSizes.size())
			.setPPoolSizes(poolSizes.data());

		descPool = deviceWrapper.logicalDevice.createDescriptorPool(info);
	}
	void imgui_init_vulkan(DeviceWrapper& deviceWrapper, RingBuffer& ringBuffer, Window& window, vk::RenderPass& renderPass)
	{
		struct ImGui_ImplVulkan_InitInfo info = { 0 };
		info.Instance = window.get_vulkan_instance();
		info.PhysicalDevice = deviceWrapper.physicalDevice;
		info.Device = deviceWrapper.logicalDevice;
		info.QueueFamily = deviceWrapper.iQueue;
		info.Queue = deviceWrapper.queue;
		info.PipelineCache = nullptr;
		info.DescriptorPool = descPool;
		info.Subpass = 1;
		info.MinImageCount = (uint32_t)ringBuffer.frames.size(); // TODO: can prolly remove this one
		info.ImageCount = (uint32_t)ringBuffer.frames.size();
		info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;

		ImGui_ImplVulkan_Init(&info, renderPass);
	}
	void imgui_upload_fonts(DeviceWrapper& deviceWrapper, RingBuffer& ringBuffer)
	{
		vk::CommandBuffer commandBuffer = ringBuffer.frames[0].commandBuffer;
		deviceWrapper.logicalDevice.resetCommandPool(ringBuffer.frames[0].commandPool);

		vk::CommandBufferBeginInfo beginInfo = vk::CommandBufferBeginInfo()
			.setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
		commandBuffer.begin(beginInfo);

		// upload fonts
		ImGui_ImplVulkan_CreateFontsTexture(commandBuffer);

		commandBuffer.end();
		vk::SubmitInfo submitInfo = vk::SubmitInfo()
			.setCommandBufferCount(1)
			.setPCommandBuffers(&commandBuffer);
		deviceWrapper.queue.submit(submitInfo);

		deviceWrapper.logicalDevice.waitIdle();
		ImGui_ImplVulkan_DestroyFontUploadObjects();
	}

private:
	vk::DescriptorPool descPool;
};