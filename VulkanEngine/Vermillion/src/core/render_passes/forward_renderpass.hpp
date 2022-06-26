#pragma once

struct ForwardRenderpassCreateInfo
{
	DeviceWrapper& deviceWrapper;
	SwapchainWrapper& swapchainWrapper;
	vma::Allocator& allocator;
	vk::DescriptorPool& descPool;
};

class ForwardRenderpass
{
public:
	ForwardRenderpass() = default;
	~ForwardRenderpass() = default;
	ROF_COPY_MOVE_DELETE(ForwardRenderpass)

public:
	void init(ForwardRenderpassCreateInfo& info)
	{
		// TODO
	}
	void destroy(DeviceWrapper& deviceWrapper, vma::Allocator& allocator)
	{
		// TODO
	}



private:
	vk::RenderPass renderPass;

	// subpasses
	vk::Pipeline geometryPassPipeline, lightingPassPipeline;
	vk::PipelineLayout geometryPassPipelineLayout, lightingPassPipelineLayout;
	vk::PipelineCache geometryPassPipelineCache, lightingPassPipelineCache; // TODO

	// shaders for the subpasses
	vk::ShaderModule geometryVS, geometryPS;
	vk::ShaderModule lightingVS, lightingPS;

	// render resources
	GBuffer gbuffer;
	vk::Framebuffer framebuffer;

	// misc
	vk::Rect2D fullscreenRect;
	std::array<vk::ClearValue, 5> clearValues;



	// image
	static constexpr vk::Format colorFormat = vk::Format::eR8G8B8A8Srgb;
	vma::Allocation posAlloc;
	vk::Image posImage;
	vk::ImageView posImageView;

	vk::DescriptorSet descSet;
	vk::DescriptorSetLayout descSetLayout;
};