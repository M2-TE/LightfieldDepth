#pragma once

class Lightfield
{
public:
	Lightfield() = default;
	~Lightfield() = default;
	ROF_COPY_MOVE_DELETE(Lightfield)

public:
	void init(ForwardRenderpassCreateInfo& info)
	{
		create_images(info.allocator, info.swapchainWrapper);
		create_image_views(info.deviceWrapper);
		create_desc_set_layout(info.deviceWrapper);
		create_desc_set(info.deviceWrapper, info.descPool);
	}
	void destroy(DeviceWrapper& deviceWrapper, vma::Allocator& allocator)
	{
		allocator.destroyImage(lightfieldImage, lightfieldAlloc);
		deviceWrapper.logicalDevice.destroyImageView(lightfieldImageView);
		for (auto i = 0u; i < nCameras; i++) {
			deviceWrapper.logicalDevice.destroyImageView(lightfieldSingleImageViews[i]);
		}

		deviceWrapper.logicalDevice.destroyDescriptorSetLayout(descSetLayout);
	}
	inline std::vector<vk::ImageView>& get_output_views()
	{
		return lightfieldSingleImageViews;
	}
	inline vk::ImageView& get_input_view()
	{
		return lightfieldImageView;
	}

private:
	// TODO: create 3d image instead of 2d array?
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
		allocator.setAllocationName(lightfieldAlloc, std::string("Lightfield").c_str());
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
		subresourceRange.setLayerCount(1);
		imageViewInfo.setViewType(vk::ImageViewType::e2D);
		lightfieldSingleImageViews.resize(nCameras);
		for (auto i = 0u; i < nCameras; i++) {
			subresourceRange.setBaseArrayLayer(i);
			imageViewInfo.setSubresourceRange(subresourceRange);
			lightfieldSingleImageViews[i] = deviceWrapper.logicalDevice.createImageView(imageViewInfo);
		}
	}
	void create_desc_set_layout(DeviceWrapper& deviceWrapper)
	{
		// one binding for each image in gbuffer
		std::array<vk::DescriptorSetLayoutBinding, 1> setLayoutBindings;
		setLayoutBindings[0]
			.setBinding(0)
			.setDescriptorCount(1)
			.setDescriptorType(vk::DescriptorType::eInputAttachment)
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
			.setDescriptorType(vk::DescriptorType::eInputAttachment)
			.setDescriptorCount(descriptors.size())
			//
			.setPBufferInfo(nullptr)
			.setPImageInfo(descriptors.data())
			.setPTexelBufferView(nullptr);

		deviceWrapper.logicalDevice.updateDescriptorSets(descBufferWrites, {});
	}

public:
	static constexpr size_t nCameras = 9;
private:
	static constexpr vk::Format colorFormat = vk::Format::eR8G8B8A8Srgb;

	vma::Allocation lightfieldAlloc;
	vk::Image lightfieldImage;
	std::vector<vk::ImageView> lightfieldSingleImageViews;
	vk::ImageView lightfieldImageView;

	vk::DescriptorSet descSet;
	vk::DescriptorSetLayout descSetLayout;
};