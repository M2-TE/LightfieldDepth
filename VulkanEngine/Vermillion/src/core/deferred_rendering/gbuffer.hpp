#pragma once

class GBuffer
{
public:
	GBuffer() = default;
	~GBuffer() = default;
	// copy operations
	GBuffer(const GBuffer& other) = default;		
	GBuffer& operator=(const GBuffer& other) = default;
	// move operations
	GBuffer(GBuffer&& other) = default;
	GBuffer& operator=(GBuffer&& other) = default;

public:
	void init(DeferredRenderpassCreateInfo& info, vk::DescriptorSetLayout& descSetLayout)
	{
		create_images(info.allocator, info.swapchainWrapper);
		create_image_views(info.deviceWrapper);
		create_desc_set(info.deviceWrapper, info.descPool, descSetLayout);
	}
	void destroy(DeviceWrapper& deviceWrapper, vma::Allocator& allocator)
	{
		allocator.destroyImage(posImage, posAlloc);
		allocator.destroyImage(colImage, colAlloc);
		allocator.destroyImage(normImage, normAlloc);
		deviceWrapper.logicalDevice.destroyImageView(posImageView);
		deviceWrapper.logicalDevice.destroyImageView(colImageView);
		deviceWrapper.logicalDevice.destroyImageView(normImageView);
	}

private:
	void create_images(vma::Allocator& allocator, SwapchainWrapper& swapchainWrapper)
	{
		vk::ImageCreateInfo imageCreateInfo = vk::ImageCreateInfo()
			.setImageType(vk::ImageType::e2D)
			.setExtent(vk::Extent3D(swapchainWrapper.extent, 1))
			//
			.setMipLevels(1)
			.setArrayLayers(1)
			.setSamples(vk::SampleCountFlagBits::e1)
			.setTiling(vk::ImageTiling::eOptimal)
			.setUsage(vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eInputAttachment);

		vma::AllocationCreateInfo allocCreateInfo = vma::AllocationCreateInfo()
			.setUsage(vma::MemoryUsage::eAutoPreferDevice)
			.setFlags(vma::AllocationCreateFlagBits::eDedicatedMemory);


		// gPosBuffer
		vk::Result result;
		imageCreateInfo.setFormat(vk::Format::eR16G16B16A16Sfloat); // 16-bit signed float
		imageCreateInfo.setFormat(vk::Format::eR32G32B32A32Sfloat); // 32-bit signed float
		result = allocator.createImage(&imageCreateInfo, &allocCreateInfo, &posImage, &posAlloc, nullptr);
		if (result != vk::Result::eSuccess) VMI_ERR("Gbuffer pos image creation unsuccessful");

		// gColBuffer
		imageCreateInfo.setFormat(vk::Format::eR8G8B8A8Srgb);
		result = allocator.createImage(&imageCreateInfo, &allocCreateInfo, &colImage, &colAlloc, nullptr);
		if (result != vk::Result::eSuccess) VMI_ERR("Gbuffer col image creation unsuccessful");

		// gNormBuffer
		imageCreateInfo.setFormat(vk::Format::eR8G8B8A8Snorm); // normalized between -1 and 1
		result = allocator.createImage(&imageCreateInfo, &allocCreateInfo, &normImage, &normAlloc, nullptr);
		if (result != vk::Result::eSuccess) VMI_ERR("Gbuffer norm image creation unsuccessful");
	}
	void create_image_views(DeviceWrapper& deviceWrapper)
	{
		vk::ImageSubresourceRange subresourceRange = vk::ImageSubresourceRange()
			.setAspectMask(vk::ImageAspectFlagBits::eColor)
			.setBaseMipLevel(0).setLevelCount(1)
			.setBaseArrayLayer(0).setLayerCount(1);

		vk::ImageViewCreateInfo imageViewInfo = vk::ImageViewCreateInfo()
			.setPNext(nullptr)
			.setViewType(vk::ImageViewType::e2D)
			.setFormat(vk::Format::eR8G8B8A8Srgb)
			.setSubresourceRange(subresourceRange);

		// gPosImageView
		imageViewInfo.setFormat(vk::Format::eR16G16B16A16Sfloat); // 16-bit signed float
		imageViewInfo.setFormat(vk::Format::eR32G32B32A32Sfloat); // 32-bit signed float
		imageViewInfo.setImage(posImage);
		posImageView = deviceWrapper.logicalDevice.createImageView(imageViewInfo);

		// gColImageView
		imageViewInfo.setFormat(vk::Format::eR8G8B8A8Srgb);
		imageViewInfo.setImage(colImage);
		colImageView = deviceWrapper.logicalDevice.createImageView(imageViewInfo);

		// gNormImageView
		imageViewInfo.setFormat(vk::Format::eR8G8B8A8Snorm); // normalized between -1 and 1
		imageViewInfo.setImage(normImage);
		normImageView = deviceWrapper.logicalDevice.createImageView(imageViewInfo);
	}
	void create_desc_set(DeviceWrapper& deviceWrapper, vk::DescriptorPool& descPool, vk::DescriptorSetLayout& descSetLayout)
	{
		// allocate the descriptor sets using descriptor pool
		vk::DescriptorSetAllocateInfo allocInfo = vk::DescriptorSetAllocateInfo()
			.setDescriptorPool(descPool)
			.setDescriptorSetCount(1).setPSetLayouts(&descSetLayout);
		descSet = deviceWrapper.logicalDevice.allocateDescriptorSets(allocInfo)[0];

		std::array<vk::DescriptorImageInfo, nImages> descriptors;
		// gPos image
		descriptors[0]
			.setImageLayout(vk::ImageLayout::eShaderReadOnlyOptimal)
			.setImageView(posImageView)
			.setSampler(nullptr);
		// gCol image
		descriptors[1]
			.setImageLayout(vk::ImageLayout::eShaderReadOnlyOptimal)
			.setImageView(colImageView)
			.setSampler(nullptr);
		// gNorm image
		descriptors[2]
			.setImageLayout(vk::ImageLayout::eShaderReadOnlyOptimal)
			.setImageView(normImageView)
			.setSampler(nullptr);

		// gPos desc set
		vk::WriteDescriptorSet descBufferWrites = vk::WriteDescriptorSet()
			.setDstSet(descSet)
			.setDstBinding(0)
			.setDstArrayElement(0)
			.setDescriptorType(vk::DescriptorType::eInputAttachment)
			.setDescriptorCount(descriptors.size())
			//
			.setPBufferInfo(nullptr)
			.setPImageInfo(descriptors.data())
			.setPTexelBufferView(nullptr);

		deviceWrapper.logicalDevice.updateDescriptorSets(descBufferWrites, {});
	}

public:
	static constexpr size_t nImages = 3;
private:
	friend class DeferredRenderpass;
	vma::Allocation posAlloc, colAlloc, normAlloc;
	vk::Image posImage, colImage, normImage;
	vk::ImageView posImageView, colImageView, normImageView;
	vk::DescriptorSet descSet;
};