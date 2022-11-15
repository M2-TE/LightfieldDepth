#pragma once

#include "stb_image.h"

struct LightfieldCreateInfo
{
	DeviceWrapper& deviceWrapper;
	SwapchainWrapper& swapchainWrapper;
	vma::Allocator& allocator;
	vk::DescriptorPool& descPool;
	vk::CommandPool& commandPool;
	std::string srcFolder;
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
		load_images(info.deviceWrapper, info.allocator, info.commandPool, info.srcFolder);
		create_desc_set_layout(info.deviceWrapper);
		create_desc_set(info.deviceWrapper, info.descPool);
	}
	void destroy(DeviceWrapper& deviceWrapper, vma::Allocator& allocator)
	{
		allocator.destroyImage(lightfieldImage, lightfieldAlloc);
		allocator.destroyImage(gradientsImage, gradientsAlloc);
		allocator.destroyImage(disparityImage, disparityAlloc);
		allocator.destroyImage(comparisonImage, comparisonAlloc);

		deviceWrapper.logicalDevice.destroyImageView(lightfieldImageView);
		deviceWrapper.logicalDevice.destroyImageView(gradientsImageView);
		deviceWrapper.logicalDevice.destroyImageView(disparityImageView);
		deviceWrapper.logicalDevice.destroyImageView(comparisonImageView);
		deviceWrapper.logicalDevice.destroySampler(samplerLightfields);
		deviceWrapper.logicalDevice.destroySampler(samplerGradients);

		for (auto i = 0u; i < nCameras; i++) {
			deviceWrapper.logicalDevice.destroyImageView(lightfieldSingleImageViews[i]);
		}

		deviceWrapper.logicalDevice.destroyDescriptorSetLayout(descSetLayoutSingle);
		deviceWrapper.logicalDevice.destroyDescriptorSetLayout(descSetLayoutDouble);
	}
	void load_images(DeviceWrapper& deviceWrapper, vma::Allocator& allocator, vk::CommandPool& commandPool, std::string srcFolder = "")
	{
		if (srcFolder == "") srcFolder = srcFolderCache;
		else srcFolderCache = srcFolder;
		
		std::string folder;
		load_image_data(folder.assign(srcFolder).append("input_Cam039.png").c_str(), 0, deviceWrapper, allocator, commandPool);
		load_image_data(folder.assign(srcFolder).append("input_Cam048.png").c_str(), 1, deviceWrapper, allocator, commandPool);
		load_image_data(folder.assign(srcFolder).append("input_Cam057.png").c_str(), 2, deviceWrapper, allocator, commandPool);
		load_image_data(folder.assign(srcFolder).append("input_Cam040.png").c_str(), 3, deviceWrapper, allocator, commandPool);
		load_image_data(folder.assign(srcFolder).append("input_Cam049.png").c_str(), 4, deviceWrapper, allocator, commandPool);
		load_image_data(folder.assign(srcFolder).append("input_Cam058.png").c_str(), 5, deviceWrapper, allocator, commandPool);
		load_image_data(folder.assign(srcFolder).append("input_Cam041.png").c_str(), 6, deviceWrapper, allocator, commandPool);
		load_image_data(folder.assign(srcFolder).append("input_Cam050.png").c_str(), 7, deviceWrapper, allocator, commandPool);
		load_image_data(folder.assign(srcFolder).append("input_Cam059.png").c_str(), 8, deviceWrapper, allocator, commandPool);

		//load_comparison_image_data(folder.assign(srcFolder).append("gt_disp_lowres.pfm").c_str(), deviceWrapper, allocator, commandPool);
		load_comparison_image_data(folder.assign(srcFolder).append("gt_depth_lowres.pfm").c_str(), deviceWrapper, allocator, commandPool);
	}
	void layout_transition_lightfields(vk::CommandBuffer& commandBuffer, vk::ImageLayout from, vk::ImageLayout to)
	{
		// change all 9 images
		for (int i = 0; i < 9; i++) {
			vk::ImageMemoryBarrier barrier = vk::ImageMemoryBarrier()
				.setOldLayout(from)
				.setNewLayout(to)
				.setImage(lightfieldImage)
				.setSubresourceRange(vk::ImageSubresourceRange()
					.setAspectMask(vk::ImageAspectFlagBits::eColor)
					.setBaseArrayLayer(i).setLayerCount(1)
					.setBaseMipLevel(0).setLevelCount(1));
			commandBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eTopOfPipe, vk::PipelineStageFlagBits::eTransfer, {}, {}, {}, barrier);
		}
	}
	void layout_transition_gradients(vk::CommandBuffer& commandBuffer, vk::ImageLayout from, vk::ImageLayout to)
	{
		vk::ImageMemoryBarrier barrier = vk::ImageMemoryBarrier()
			.setOldLayout(from)
			.setNewLayout(to)
			.setImage(gradientsImage)
			.setSubresourceRange(vk::ImageSubresourceRange()
				.setAspectMask(vk::ImageAspectFlagBits::eColor)
				.setBaseArrayLayer(0).setLayerCount(1)
				.setBaseMipLevel(0).setLevelCount(1));
		commandBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eTopOfPipe, vk::PipelineStageFlagBits::eTransfer, {}, {}, {}, barrier);
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
		imageCreateInfo.setUsage(vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled);
		result = allocator.createImage(&imageCreateInfo, &allocCreateInfo, &gradientsImage, &gradientsAlloc, nullptr);
		if (result != vk::Result::eSuccess) VMI_ERR("Gradients image creation unsuccessful");
		allocator.setAllocationName(gradientsAlloc, std::string("Gradients").c_str());

		// comparison
		imageCreateInfo.setUsage(vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferDst);
		result = allocator.createImage(&imageCreateInfo, &allocCreateInfo, &comparisonImage, &comparisonAlloc, nullptr);
		if (result != vk::Result::eSuccess) VMI_ERR("Gradients image creation unsuccessful");
		allocator.setAllocationName(comparisonAlloc, std::string("Comparison").c_str());

		// disparity
		imageCreateInfo.setUsage(vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eInputAttachment);
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
		imageViewInfo.setImage(gradientsImage);
		gradientsImageView = deviceWrapper.logicalDevice.createImageView(imageViewInfo);

		// comparison view
		imageViewInfo.setImage(comparisonImage);
		comparisonImageView = deviceWrapper.logicalDevice.createImageView(imageViewInfo);

		// disparity view
		imageViewInfo.setImage(disparityImage);
		disparityImageView = deviceWrapper.logicalDevice.createImageView(imageViewInfo);
	}
	void load_image_data(const char* filename, uint32_t iCam, DeviceWrapper& deviceWrapper, vma::Allocator& allocator, vk::CommandPool& commandPool)
	{
		int x, y, n;
		auto* img = stbi_load(filename, &x, &y, &n, STBI_rgb_alpha);
		if (!img) {
			VMI_ERR("Error on img load: Camera " << iCam << " with path: " << filename);
			switch (iCam) {
			case 0: img = stbi_load("lightfields/training/cotton/input_Cam039.png", &x, &y, &n, STBI_rgb_alpha); break;
			case 1: img = stbi_load("lightfields/training/cotton/input_Cam048.png", &x, &y, &n, STBI_rgb_alpha); break;
			case 2: img = stbi_load("lightfields/training/cotton/input_Cam057.png", &x, &y, &n, STBI_rgb_alpha); break;
			case 3: img = stbi_load("lightfields/training/cotton/input_Cam040.png", &x, &y, &n, STBI_rgb_alpha); break;
			case 4: img = stbi_load("lightfields/training/cotton/input_Cam049.png", &x, &y, &n, STBI_rgb_alpha); break;
			case 5: img = stbi_load("lightfields/training/cotton/input_Cam058.png", &x, &y, &n, STBI_rgb_alpha); break;
			case 6: img = stbi_load("lightfields/training/cotton/input_Cam041.png", &x, &y, &n, STBI_rgb_alpha); break;
			case 7: img = stbi_load("lightfields/training/cotton/input_Cam050.png", &x, &y, &n, STBI_rgb_alpha); break;
			case 8: img = stbi_load("lightfields/training/cotton/input_Cam059.png", &x, &y, &n, STBI_rgb_alpha); break;
			}
		}
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
			//barrier.newLayout = vk::ImageLayout::eColorAttachmentOptimal;
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
	void load_comparison_image_data(const char* filename, DeviceWrapper& deviceWrapper, vma::Allocator& allocator, vk::CommandPool& commandPool)
	{
		// reading grayscale .pfm file
		std::ifstream myfile(filename, std::ios::binary);
		std::streampos begin = myfile.tellg();
		myfile.seekg(0, std::ios::end);
		std::streampos end = myfile.tellg();
		std::vector<float> imgData((end - begin) / sizeof(float));
		myfile.seekg(0, std::ios::beg);
		myfile.read(reinterpret_cast<char*>(imgData.data()), end - begin);
		myfile.close();

		float* img = imgData.data() + 3; // 3x4 bytes header
		int x = 512;
		int y = 512;
		if (!img) {
			VMI_ERR("Error on img load: Comparison image with path: " << filename);
		}
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
					.setBaseArrayLayer(0)
					.setLayerCount(1)
					.setMipLevel(0));

			vk::ImageMemoryBarrier barrier = vk::ImageMemoryBarrier()
				.setOldLayout(vk::ImageLayout::eUndefined)
				.setNewLayout(vk::ImageLayout::eTransferDstOptimal)
				.setImage(comparisonImage)
				.setSubresourceRange(vk::ImageSubresourceRange()
					.setAspectMask(vk::ImageAspectFlagBits::eColor)
					.setBaseArrayLayer(0)
					.setLayerCount(1)
					.setBaseMipLevel(0)
					.setLevelCount(1));
			commandBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eTopOfPipe, vk::PipelineStageFlagBits::eTransfer, {}, {}, {}, barrier);
			commandBuffer.copyBufferToImage(stagingBuffer.first, comparisonImage, vk::ImageLayout::eTransferDstOptimal, region);
			barrier.oldLayout = vk::ImageLayout::eTransferDstOptimal;
			barrier.newLayout = vk::ImageLayout::eReadOnlyOptimal;
			//barrier.newLayout = vk::ImageLayout::eColorAttachmentOptimal;
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
	}

	void create_desc_set_layout(DeviceWrapper& deviceWrapper)
	{
		// one binding for each image in gbuffer
		std::array<vk::DescriptorSetLayoutBinding, 2> setLayoutBindings;
		setLayoutBindings[0]
			.setBinding(0)
			.setDescriptorCount(1)
			.setDescriptorType(vk::DescriptorType::eCombinedImageSampler)
			.setStageFlags(vk::ShaderStageFlagBits::eFragment);
		setLayoutBindings[1]
			.setBinding(1)
			.setDescriptorCount(1)
			.setDescriptorType(vk::DescriptorType::eCombinedImageSampler)
			.setStageFlags(vk::ShaderStageFlagBits::eFragment);


		// create descriptor set layout from the bindings
		vk::DescriptorSetLayoutCreateInfo createInfo = vk::DescriptorSetLayoutCreateInfo()
			.setBindingCount((uint32_t)setLayoutBindings.size())
			.setPBindings(setLayoutBindings.data());
		descSetLayoutDouble = deviceWrapper.logicalDevice.createDescriptorSetLayout(createInfo);

		createInfo = vk::DescriptorSetLayoutCreateInfo()
			.setBindingCount(1u)
			.setPBindings(setLayoutBindings.data());
		descSetLayoutSingle = deviceWrapper.logicalDevice.createDescriptorSetLayout(createInfo);
	}
	void create_desc_set(DeviceWrapper& deviceWrapper, vk::DescriptorPool& descPool)
	{
		// lightfield images
		{
			// allocate the descriptor sets using descriptor pool
			vk::DescriptorSetAllocateInfo allocInfo = vk::DescriptorSetAllocateInfo()
				.setDescriptorPool(descPool)
				.setDescriptorSetCount(1).setPSetLayouts(&descSetLayoutSingle);
			descSetLightfield = deviceWrapper.logicalDevice.allocateDescriptorSets(allocInfo)[0];

			// create sampler for images
			vk::SamplerCreateInfo samplerInfo = vk::SamplerCreateInfo()
				.setMagFilter(vk::Filter::eNearest)
				.setMinFilter(vk::Filter::eNearest)
				.setAnisotropyEnable(VK_FALSE) // not needed for now
				.setMaxAnisotropy(0.0f)
				.setUnnormalizedCoordinates(VK_FALSE)
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
			samplerLightfields = deviceWrapper.logicalDevice.createSampler(samplerInfo);

			std::array<vk::DescriptorImageInfo, 1> descriptors;
			descriptors[0]
				.setImageLayout(vk::ImageLayout::eShaderReadOnlyOptimal)
				.setImageView(lightfieldImageView)
				.setSampler(samplerLightfields);

			// desc set
			vk::WriteDescriptorSet descBufferWrites = vk::WriteDescriptorSet()
				.setDstSet(descSetLightfield)
				.setDstBinding(0)
				.setDstArrayElement(0)
				.setDescriptorType(vk::DescriptorType::eCombinedImageSampler)
				.setDescriptorCount((uint32_t)descriptors.size())
				//
				.setPBufferInfo(nullptr)
				.setPImageInfo(descriptors.data())
				.setPTexelBufferView(nullptr);

			deviceWrapper.logicalDevice.updateDescriptorSets(descBufferWrites, {});
		}

		// gradient image
		{
			// allocate the descriptor sets using descriptor pool
			vk::DescriptorSetAllocateInfo allocInfo = vk::DescriptorSetAllocateInfo()
				.setDescriptorPool(descPool)
				.setDescriptorSetCount(1).setPSetLayouts(&descSetLayoutDouble);
			descSetGradients = deviceWrapper.logicalDevice.allocateDescriptorSets(allocInfo)[0];

			// create sampler for images
			vk::SamplerCreateInfo samplerInfo = vk::SamplerCreateInfo()
				.setMagFilter(vk::Filter::eNearest)
				.setMinFilter(vk::Filter::eNearest)
				.setAnisotropyEnable(VK_FALSE) // not needed for now
				.setMaxAnisotropy(0.0f)
				.setUnnormalizedCoordinates(VK_FALSE)
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
			samplerGradients = deviceWrapper.logicalDevice.createSampler(samplerInfo);

			std::array<vk::DescriptorImageInfo, 2> descriptors;
			descriptors[0]
				.setImageLayout(vk::ImageLayout::eShaderReadOnlyOptimal)
				.setImageView(gradientsImageView)
				.setSampler(samplerGradients);
			descriptors[1]
				.setImageLayout(vk::ImageLayout::eShaderReadOnlyOptimal)
				.setImageView(comparisonImageView)
				.setSampler(samplerGradients);

			// desc set
			vk::WriteDescriptorSet descBufferWrites = vk::WriteDescriptorSet()
				.setDstSet(descSetGradients)
				.setDstBinding(0)
				.setDstArrayElement(0)
				.setDescriptorType(vk::DescriptorType::eCombinedImageSampler)
				.setDescriptorCount((uint32_t)descriptors.size())
				//
				.setPBufferInfo(nullptr)
				.setPImageInfo(descriptors.data())
				.setPTexelBufferView(nullptr);

			deviceWrapper.logicalDevice.updateDescriptorSets(descBufferWrites, {});
		}
	}

public:
	static constexpr size_t nCameras = 9;
	static constexpr vk::Format colorFormat = vk::Format::eR8G8B8A8Srgb;

	vma::Allocation lightfieldAlloc, gradientsAlloc, disparityAlloc, comparisonAlloc;
	vk::Image lightfieldImage, gradientsImage, disparityImage, comparisonImage;
	vk::ImageView lightfieldImageView, gradientsImageView, disparityImageView, comparisonImageView;
	std::vector<vk::ImageView> lightfieldSingleImageViews; // one view for each cam to render into

	vk::DescriptorSetLayout descSetLayoutSingle;
	vk::DescriptorSetLayout descSetLayoutDouble;
	vk::DescriptorSet descSetLightfield, descSetGradients;
	vk::Sampler samplerLightfields, samplerGradients;
	std::string srcFolderCache;
};