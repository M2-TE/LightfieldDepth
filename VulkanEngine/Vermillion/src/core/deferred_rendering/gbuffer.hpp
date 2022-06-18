#pragma once

class GBuffer
{
public:
	GBuffer() = default;
	~GBuffer() = default;
	ROF_COPY_MOVE_DELETE(GBuffer)

public:
	void init(DeferredRenderpassCreateInfo& info)
	{
		create_images(info.allocator, info.swapchainWrapper);
		create_image_views(info.deviceWrapper);
		create_desc_set_layout(info.deviceWrapper);
		create_desc_set(info.deviceWrapper, info.descPool);
	}
	void destroy(DeviceWrapper& deviceWrapper, vma::Allocator& allocator)
	{
		allocator.destroyImage(posImage, posAlloc);
		allocator.destroyImage(colImage, colAlloc);
		allocator.destroyImage(normImage, normAlloc);
		allocator.destroyImage(outputImage, outputAlloc);
		allocator.destroyImage(depthStencilImage, depthStencilAlloc);
		deviceWrapper.logicalDevice.destroyImageView(posImageView);
		deviceWrapper.logicalDevice.destroyImageView(colImageView);
		deviceWrapper.logicalDevice.destroyImageView(normImageView);
		deviceWrapper.logicalDevice.destroyImageView(outputImageView);
		deviceWrapper.logicalDevice.destroyImageView(depthStencilView);
		deviceWrapper.logicalDevice.destroyDescriptorSetLayout(descSetLayout);
	}

private:
	void create_images(vma::Allocator& allocator, SwapchainWrapper& swapchainWrapper)
	{
		vk::ImageCreateInfo imageCreateInfo = vk::ImageCreateInfo()
			.setImageType(vk::ImageType::e2D)
			.setExtent(vk::Extent3D(swapchainWrapper.extent, 1))
			//
			.setMipLevels(1).setArrayLayers(1)
			.setSamples(vk::SampleCountFlagBits::e1)
			.setTiling(vk::ImageTiling::eOptimal)
			.setUsage(vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eInputAttachment);

		vma::AllocationCreateInfo allocCreateInfo = vma::AllocationCreateInfo()
			.setUsage(vma::MemoryUsage::eAutoPreferDevice)
			.setFlags(vma::AllocationCreateFlagBits::eDedicatedMemory);

		// position
		vk::Result result;
		imageCreateInfo.setFormat(vk::Format::eR16G16B16A16Sfloat); // 16-bit signed float
		imageCreateInfo.setFormat(vk::Format::eR32G32B32A32Sfloat); // 32-bit signed float
		result = allocator.createImage(&imageCreateInfo, &allocCreateInfo, &posImage, &posAlloc, nullptr);
		if (result != vk::Result::eSuccess) VMI_ERR("Gbuffer position image creation unsuccessful");
		allocator.setAllocationName(posAlloc, std::string("GBuffer positions").c_str());

		// color
		imageCreateInfo.setFormat(colorFormat);
		result = allocator.createImage(&imageCreateInfo, &allocCreateInfo, &colImage, &colAlloc, nullptr);
		if (result != vk::Result::eSuccess) VMI_ERR("GBuffer color image creation unsuccessful");
		allocator.setAllocationName(colAlloc, std::string("GBuffer colors").c_str());

		// normals
		imageCreateInfo.setFormat(vk::Format::eR8G8B8A8Snorm); // normalized between -1 and 1
		result = allocator.createImage(&imageCreateInfo, &allocCreateInfo, &normImage, &normAlloc, nullptr);
		if (result != vk::Result::eSuccess) VMI_ERR("GBuffer normal image creation unsuccessful");
		allocator.setAllocationName(normAlloc, std::string("GBuffer normals").c_str());

		// output
		imageCreateInfo.setFormat(colorFormat);
		imageCreateInfo.setUsage(vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eColorAttachment);
		VMI_LOG("Note: Deferred output still marked as blit src");
		result = allocator.createImage(&imageCreateInfo, &allocCreateInfo, &outputImage, &outputAlloc, nullptr);
		if (result != vk::Result::eSuccess) VMI_ERR("GBuffer output image creation unsuccessful");
		allocator.setAllocationName(outputAlloc, std::string("GBuffer output").c_str());

		// depth stencil
		imageCreateInfo.setFormat(vk::Format::eD24UnormS8Uint);
		imageCreateInfo.setUsage(vk::ImageUsageFlagBits::eDepthStencilAttachment);
		result = allocator.createImage(&imageCreateInfo, &allocCreateInfo, &depthStencilImage, &depthStencilAlloc, nullptr);
		if (result != vk::Result::eSuccess) VMI_ERR("Depth Stencil creation unsuccessful");
		allocator.setAllocationName(depthStencilAlloc, std::string("Depth Stencil").c_str());
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

		// positions view
		imageViewInfo.setFormat(vk::Format::eR16G16B16A16Sfloat); // 16-bit signed float
		imageViewInfo.setFormat(vk::Format::eR32G32B32A32Sfloat); // 32-bit signed float
		imageViewInfo.setImage(posImage);
		posImageView = deviceWrapper.logicalDevice.createImageView(imageViewInfo);

		// colors view
		imageViewInfo.setFormat(colorViewFormat);
		imageViewInfo.setImage(colImage);
		colImageView = deviceWrapper.logicalDevice.createImageView(imageViewInfo);

		// normals view
		imageViewInfo.setFormat(vk::Format::eR8G8B8A8Snorm); // normalized between -1 and 1
		imageViewInfo.setImage(normImage);
		normImageView = deviceWrapper.logicalDevice.createImageView(imageViewInfo);

		// output view
		imageViewInfo.setFormat(colorViewFormat);
		imageViewInfo.setImage(outputImage);
		outputImageView = deviceWrapper.logicalDevice.createImageView(imageViewInfo);

		// depth stencil view
		subresourceRange.setAspectMask(vk::ImageAspectFlagBits::eDepth | vk::ImageAspectFlagBits::eStencil);
		imageViewInfo .setFormat(vk::Format::eD24UnormS8Uint).setSubresourceRange(subresourceRange);
		imageViewInfo.setImage(depthStencilImage);
		depthStencilView = deviceWrapper.logicalDevice.createImageView(imageViewInfo);
	}
	void create_desc_set(DeviceWrapper& deviceWrapper, vk::DescriptorPool& descPool)
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
	void create_desc_set_layout(DeviceWrapper& deviceWrapper)
	{
		// one binding for each image in gbuffer
		std::array<vk::DescriptorSetLayoutBinding, GBuffer::nImages> setLayoutBindings;
		for (size_t i = 0; i < setLayoutBindings.size(); i++)
		{
			setLayoutBindings[i]
				.setBinding(i)
				.setDescriptorCount(1)
				.setDescriptorType(vk::DescriptorType::eInputAttachment)
				.setStageFlags(vk::ShaderStageFlagBits::eFragment);
		}

		// create descriptor set layout from the bindings
		vk::DescriptorSetLayoutCreateInfo createInfo = vk::DescriptorSetLayoutCreateInfo()
			.setBindingCount(setLayoutBindings.size())
			.setPBindings(setLayoutBindings.data());
		descSetLayout = deviceWrapper.logicalDevice.createDescriptorSetLayout(createInfo);
	}

public:
	static constexpr size_t nImages = 3; // pos, col, norm
private:
	friend class DeferredRenderpass;
	static constexpr vk::Format colorFormat = vk::Format::eR8G8B8A8Srgb;
	static constexpr vk::Format colorViewFormat = vk::Format::eR8G8B8A8Srgb;

	vma::Allocation posAlloc, colAlloc, normAlloc, outputAlloc, depthStencilAlloc;
	vk::Image posImage, colImage, normImage, outputImage, depthStencilImage;
	vk::ImageView posImageView, colImageView, normImageView, outputImageView, depthStencilView;

	vk::DescriptorSet descSet;
	vk::DescriptorSetLayout descSetLayout;
};