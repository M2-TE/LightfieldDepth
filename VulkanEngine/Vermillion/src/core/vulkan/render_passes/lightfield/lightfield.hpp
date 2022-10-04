#pragma once

#include "stb_image.h"

struct LightfieldCreateInfo
{
	DeviceWrapper& deviceWrapper;
	SwapchainWrapper& swapchainWrapper;
	vma::Allocator& allocator;
	vk::DescriptorPool& descPool;
	vk::CommandPool& commandPool;
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
		load_image_data("cotton/input_Cam000.png", 0, info.deviceWrapper, info.allocator, info.commandPool);
		load_image_data("cotton/input_Cam001.png", 1, info.deviceWrapper, info.allocator, info.commandPool);
		load_image_data("cotton/input_Cam002.png", 2, info.deviceWrapper, info.allocator, info.commandPool);
		load_image_data("cotton/input_Cam003.png", 3, info.deviceWrapper, info.allocator, info.commandPool);
		load_image_data("cotton/input_Cam004.png", 4, info.deviceWrapper, info.allocator, info.commandPool);
		load_image_data("cotton/input_Cam005.png", 5, info.deviceWrapper, info.allocator, info.commandPool);
		load_image_data("cotton/input_Cam006.png", 6, info.deviceWrapper, info.allocator, info.commandPool);
		load_image_data("cotton/input_Cam007.png", 7, info.deviceWrapper, info.allocator, info.commandPool);
		load_image_data("cotton/input_Cam008.png", 8, info.deviceWrapper, info.allocator, info.commandPool);
		create_desc_set_layout(info.deviceWrapper);
		create_desc_set(info.deviceWrapper, info.descPool);
	}
	void destroy(DeviceWrapper& deviceWrapper, vma::Allocator& allocator)
	{
		allocator.destroyImage(lightfieldImage, lightfieldAlloc);
		allocator.destroyImage(gradientsImage, gradientsAlloc);
		allocator.destroyImage(disparityImage, disparityAlloc);

		deviceWrapper.logicalDevice.destroyImageView(lightfieldImageView);
		deviceWrapper.logicalDevice.destroyImageView(gradientsImageView);
		deviceWrapper.logicalDevice.destroyImageView(disparityImageView);
		deviceWrapper.logicalDevice.destroySampler(sampler);

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
			.setUsage(vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferDst)
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
		imageCreateInfo.setUsage(vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eInputAttachment);
		result = allocator.createImage(&imageCreateInfo, &allocCreateInfo, &gradientsImage, &gradientsAlloc, nullptr);
		if (result != vk::Result::eSuccess) VMI_ERR("Gradients image creation unsuccessful");
		allocator.setAllocationName(gradientsAlloc, std::string("Gradients").c_str());

		// disparity
		imageCreateInfo.setFormat(colorFormat);
		result = allocator.createImage(&imageCreateInfo, &allocCreateInfo, &disparityImage, &disparityAlloc, nullptr);
		if (result != vk::Result::eSuccess) VMI_ERR("Disparity image creation unsuccessful");
		allocator.setAllocationName(disparityAlloc, std::string("Disparity Map").c_str());
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
	void layout_transition(vk::CommandPool commandPool, vk::ImageLayout from, vk::ImageLayout to)
	{
		// TODO
	}
	void load_image_data(const char* filename, uint32_t iCam, DeviceWrapper& deviceWrapper, vma::Allocator& allocator, vk::CommandPool& commandPool)
	{
		int x, y, n;
		auto* img = stbi_load(filename, &x, &y, &n, STBI_rgb_alpha);
		if (!img) VMI_ERR("Error on img load");
		vk::DeviceSize fileSize = x * y * STBI_rgb_alpha;

		// staging buffer
		vk::BufferCreateInfo bufferInfo = vk::BufferCreateInfo()
			.setSize(fileSize)
			.setUsage(vk::BufferUsageFlagBits::eTransferSrc);
		vma::AllocationCreateInfo allocCreateInfo = vma::AllocationCreateInfo()
			.setUsage(vma::MemoryUsage::eAuto)
			.setFlags(vma::AllocationCreateFlagBits::eHostAccessSequentialWrite | vma::AllocationCreateFlagBits::eMapped);
		vma::AllocationInfo allocInfo;
		auto stagingBuffer = allocator.createBuffer(bufferInfo, allocCreateInfo, allocInfo);

		// already mapped, so just copy over
		memcpy(allocInfo.pMappedData, img, fileSize);

		// copy from staging buffer to image
		// memory transfer
		{
			vk::CommandBufferAllocateInfo allocInfo = vk::CommandBufferAllocateInfo()
				.setLevel(vk::CommandBufferLevel::ePrimary)
				.setCommandPool(commandPool)
				.setCommandBufferCount(1);

			vk::CommandBuffer commandBuffer;
			auto res = deviceWrapper.logicalDevice.allocateCommandBuffers(&allocInfo, &commandBuffer);

			// begin recording to temporary command buffer
			vk::CommandBufferBeginInfo beginInfo = vk::CommandBufferBeginInfo()
				.setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
			commandBuffer.begin(beginInfo);

			// TODO: describe subresource here, need to select each image in lightfield array individually
			vk::BufferImageCopy region = vk::BufferImageCopy()
				// buffer
				.setBufferRowLength(x)
				.setBufferImageHeight(y)
				.setBufferOffset(0)
				// img
				.setImageExtent(vk::Extent3D(x, y, 1))
				.setImageOffset(0)
				.setImageSubresource(vk::ImageSubresourceLayers()
					.setAspectMask(vk::ImageAspectFlagBits::eColor)
					.setBaseArrayLayer(iCam)
					.setLayerCount(1)
					.setMipLevel(0));

			vk::ImageMemoryBarrier barrier = vk::ImageMemoryBarrier()
				.setOldLayout(vk::ImageLayout::eUndefined)
				.setNewLayout(vk::ImageLayout::eTransferDstOptimal)
				.setImage(lightfieldImage)
				.setSubresourceRange(vk::ImageSubresourceRange()
					.setAspectMask(vk::ImageAspectFlagBits::eColor)
					.setBaseArrayLayer(iCam)
					.setLayerCount(1)
					.setBaseMipLevel(0)
					.setLevelCount(1));
			commandBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eTopOfPipe, vk::PipelineStageFlagBits::eTransfer, {}, {}, {}, barrier);
			commandBuffer.copyBufferToImage(stagingBuffer.first, lightfieldImage, vk::ImageLayout::eTransferDstOptimal, region);
			barrier.oldLayout = vk::ImageLayout::eTransferDstOptimal;
			barrier.newLayout = vk::ImageLayout::eReadOnlyOptimal;
			commandBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eTopOfPipe, vk::PipelineStageFlagBits::eTransfer, {}, {}, {}, barrier);
			commandBuffer.end();

			vk::SubmitInfo submitInfo = vk::SubmitInfo()
				.setCommandBufferCount(1)
				.setPCommandBuffers(&commandBuffer);
			deviceWrapper.queue.submit(submitInfo);
			deviceWrapper.queue.waitIdle(); // TODO: change this to wait on a fence instead (upon queue submit) so multiple memory transfers would be possible

			// free command buffer directly after use
			deviceWrapper.logicalDevice.freeCommandBuffers(commandPool, commandBuffer);
		}

		// clean up
		allocator.destroyBuffer(stagingBuffer.first, stagingBuffer.second);
		stbi_image_free(img);
	}

	void create_desc_set_layout(DeviceWrapper& deviceWrapper)
	{
		// one binding for each image in gbuffer
		std::array<vk::DescriptorSetLayoutBinding, 1> setLayoutBindings;
		setLayoutBindings[0]
			.setBinding(0)
			.setDescriptorCount(1)
			.setDescriptorType(vk::DescriptorType::eCombinedImageSampler)
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

		// create sampler for image
		vk::SamplerCreateInfo samplerInfo = vk::SamplerCreateInfo()
			.setMagFilter(vk::Filter::eNearest)
			.setMinFilter(vk::Filter::eNearest)
			.setAnisotropyEnable(VK_FALSE) // not needed for now
			.setMaxAnisotropy(0.0f)
			.setUnnormalizedCoordinates(VK_TRUE) // makes fullscreen sampling easier
			.setBorderColor(vk::BorderColor::eIntOpaqueBlack)
			.setAddressModeU(vk::SamplerAddressMode::eClampToEdge)
			.setAddressModeV(vk::SamplerAddressMode::eClampToEdge)
			.setAddressModeW(vk::SamplerAddressMode::eClampToEdge)
			.setCompareEnable(VK_FALSE)
			.setCompareOp(vk::CompareOp::eAlways)
			.setMipmapMode(vk::SamplerMipmapMode::eNearest)
			.setMipLodBias(0.0f)
			.setMinLod(0.0f)
			.setMaxLod(0.0f);
		sampler = deviceWrapper.logicalDevice.createSampler(samplerInfo);

		std::array<vk::DescriptorImageInfo, 1> descriptors;
		descriptors[0]
			.setImageLayout(vk::ImageLayout::eShaderReadOnlyOptimal)
			.setImageView(lightfieldImageView)
			.setSampler(sampler);

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

	vk::DescriptorSetLayout descSetLayout;
	vk::DescriptorSet descSet;
	vk::Sampler sampler;
};