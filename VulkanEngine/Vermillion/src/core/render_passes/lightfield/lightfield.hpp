#pragma once

struct LightfieldCreateInfo
{
	DeviceWrapper& deviceWrapper;
	SwapchainWrapper& swapchainWrapper;
	vma::Allocator& allocator;
	vk::DescriptorPool& descPool;
};
class Lightfield
{
public:
	Lightfield() = default;
	~Lightfield() = default;
	ROF_COPY_MOVE_DELETE(Lightfield)

public:
	void init(LightfieldCreateInfo& info)
	{
		create_images(info.allocator, info.swapchainWrapper);
		create_image_views(info.deviceWrapper);
		create_sampler(info.deviceWrapper);
		create_desc_set_layout(info.deviceWrapper);
		create_desc_set(info.deviceWrapper, info.descPool);
	}
	void destroy(DeviceWrapper& deviceWrapper, vma::Allocator& allocator)
	{
		allocator.destroyImage(lightfieldImage, lightfieldAlloc);
		allocator.destroyImage(gradientsImage, gradientsAlloc);
		allocator.destroyImage(disparityImage, disparityAlloc);

		deviceWrapper.logicalDevice.destroySampler(sampler);

		deviceWrapper.logicalDevice.destroyImageView(lightfieldImageView);
		deviceWrapper.logicalDevice.destroyImageView(gradientsImageView);
		deviceWrapper.logicalDevice.destroyImageView(disparityImageView);

		for (auto i = 0u; i < nCameras; i++) {
			deviceWrapper.logicalDevice.destroyImageView(lightfieldSingleImageViews[i]);
		}

		deviceWrapper.logicalDevice.destroyDescriptorSetLayout(descSetLayout);
	}

private:
	void create_images(vma::Allocator& allocator, SwapchainWrapper& swapchainWrapper)
	{
		vk::ImageCreateInfo imageCreateInfo = vk::ImageCreateInfo()
			.setImageType(vk::ImageType::e2D)
			.setExtent(vk::Extent3D(swapchainWrapper.extent, 1))
			//
			.setMipLevels(1).setArrayLayers(nCameras)
			.setSamples(vk::SampleCountFlagBits::e1)
			.setTiling(vk::ImageTiling::eOptimal)
			.setUsage(vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eInputAttachment)
			.setFormat(colorFormat);

		vma::AllocationCreateInfo allocCreateInfo = vma::AllocationCreateInfo()
			.setUsage(vma::MemoryUsage::eAutoPreferDevice)
			.setFlags(vma::AllocationCreateFlagBits::eDedicatedMemory);

		// color
		vk::Result result = allocator.createImage(&imageCreateInfo, &allocCreateInfo, &lightfieldImage, &lightfieldAlloc, nullptr);
		if (result != vk::Result::eSuccess) VMI_ERR("Lightfield image creation unsuccessful");
		allocator.setAllocationName(lightfieldAlloc, std::string("Lightfield Array").c_str());

		// gradients
		imageCreateInfo.setArrayLayers(1);
		imageCreateInfo.setFormat(vk::Format::eR32G32B32A32Sfloat);
		result = allocator.createImage(&imageCreateInfo, &allocCreateInfo, &gradientsImage, &gradientsAlloc, nullptr);
		if (result != vk::Result::eSuccess) VMI_ERR("Gradients image creation unsuccessful");
		allocator.setAllocationName(lightfieldAlloc, std::string("Gradients").c_str());

		// disparity
		imageCreateInfo.setFormat(colorFormat);
		result = allocator.createImage(&imageCreateInfo, &allocCreateInfo, &disparityImage, &disparityAlloc, nullptr);
		if (result != vk::Result::eSuccess) VMI_ERR("Disparity image creation unsuccessful");
		allocator.setAllocationName(lightfieldAlloc, std::string("Disparity Map").c_str());
	}
	void create_image_views(DeviceWrapper& deviceWrapper)
	{
		vk::ImageSubresourceRange subresourceRange = vk::ImageSubresourceRange()
			.setAspectMask(vk::ImageAspectFlagBits::eColor)
			.setBaseMipLevel(0).setLevelCount(1)
			.setBaseArrayLayer(0).setLayerCount(nCameras);

		vk::ImageViewCreateInfo imageViewInfo = vk::ImageViewCreateInfo()
			.setPNext(nullptr)
			.setViewType(vk::ImageViewType::e2DArray)
			.setFormat(colorFormat)
			.setSubresourceRange(subresourceRange)
			.setImage(lightfieldImage);

		// colors view
		lightfieldImageView = deviceWrapper.logicalDevice.createImageView(imageViewInfo);

		// single image view for writing each image individually
		imageViewInfo.subresourceRange.layerCount = 1;
		imageViewInfo.setViewType(vk::ImageViewType::e2D);
		lightfieldSingleImageViews.resize(nCameras);
		for (auto i = 0u; i < nCameras; i++) {
			imageViewInfo.subresourceRange.baseArrayLayer = i;
			lightfieldSingleImageViews[i] = deviceWrapper.logicalDevice.createImageView(imageViewInfo);
		}

		// gradients view
		imageViewInfo.subresourceRange.baseArrayLayer = 0;
		imageViewInfo.setFormat(vk::Format::eR32G32B32A32Sfloat);
		imageViewInfo.setImage(gradientsImage);
		gradientsImageView = deviceWrapper.logicalDevice.createImageView(imageViewInfo);

		// disparity view
		imageViewInfo.setFormat(colorFormat);
		imageViewInfo.setImage(disparityImage);
		disparityImageView = deviceWrapper.logicalDevice.createImageView(imageViewInfo);
	}
	void create_sampler(DeviceWrapper& deviceWrapper)
	{
		vk::SamplerCreateInfo createInfo = vk::SamplerCreateInfo()
			.setMinFilter(vk::Filter::eLinear)
			.setMagFilter(vk::Filter::eLinear)
			.setMipmapMode(vk::SamplerMipmapMode::eLinear)
			.setAddressModeU(vk::SamplerAddressMode::eClampToEdge)
			.setAddressModeV(vk::SamplerAddressMode::eClampToEdge)
			.setAddressModeW(vk::SamplerAddressMode::eClampToEdge)
			.setAnisotropyEnable(false)
			.setCompareEnable(false)
			.setUnnormalizedCoordinates(false);

		sampler = deviceWrapper.logicalDevice.createSampler(createInfo);
	}

	void create_desc_set_layout(DeviceWrapper& deviceWrapper)
	{
		// one binding for each image in gbuffer
		std::array<vk::DescriptorSetLayoutBinding, 1> setLayoutBindings;
		setLayoutBindings[0]
			.setBinding(0)
			.setDescriptorCount(1)
			.setDescriptorType(vk::DescriptorType::eCombinedImageSampler)
			.setImmutableSamplers(sampler)
			.setStageFlags(vk::ShaderStageFlagBits::eFragment);

		// create descriptor set layout from the bindings
		vk::DescriptorSetLayoutCreateInfo createInfo = vk::DescriptorSetLayoutCreateInfo()
			.setBindingCount(setLayoutBindings.size())
			.setPBindings(setLayoutBindings.data());
		descSetLayout = deviceWrapper.logicalDevice.createDescriptorSetLayout(createInfo);
	}
	void create_desc_set(DeviceWrapper& deviceWrapper, vk::DescriptorPool& descPool)
	{
		// allocate the descriptor sets using descriptor pool
		vk::DescriptorSetAllocateInfo allocInfo = vk::DescriptorSetAllocateInfo()
			.setDescriptorPool(descPool)
			.setDescriptorSetCount(1).setPSetLayouts(&descSetLayout);
		descSet = deviceWrapper.logicalDevice.allocateDescriptorSets(allocInfo)[0];

		std::array<vk::DescriptorImageInfo, 1> descriptors;
		descriptors[0]
			.setImageLayout(vk::ImageLayout::eShaderReadOnlyOptimal)
			.setImageView(lightfieldImageView)
			.setSampler(nullptr);

		// desc set
		vk::WriteDescriptorSet descBufferWrites = vk::WriteDescriptorSet()
			.setDstSet(descSet)
			.setDstBinding(0)
			.setDstArrayElement(0)
			.setDescriptorType(vk::DescriptorType::eCombinedImageSampler)
			.setDescriptorCount(descriptors.size())
			//
			.setPBufferInfo(nullptr)
			.setPImageInfo(descriptors.data())
			.setPTexelBufferView(nullptr);

		deviceWrapper.logicalDevice.updateDescriptorSets(descBufferWrites, {});
	}

public:
	static constexpr size_t nCameras = 9;
	static constexpr vk::Format colorFormat = vk::Format::eR8G8B8A8Srgb;

	vma::Allocation lightfieldAlloc, gradientsAlloc, disparityAlloc;
	vk::Image lightfieldImage, gradientsImage, disparityImage;
	vk::ImageView lightfieldImageView, gradientsImageView, disparityImageView;
	std::vector<vk::ImageView> lightfieldSingleImageViews; // one view for each cam to render into

	// TODO: need separate desc sets for lightfield/gradients/disparity?
	vk::DescriptorSetLayout descSetLayout;
	vk::DescriptorSet descSet;
	vk::Sampler sampler;
};