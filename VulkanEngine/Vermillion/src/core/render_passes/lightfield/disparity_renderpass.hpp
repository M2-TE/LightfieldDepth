#pragma once

struct DisparityRenderpassCreateInfo
{
	DeviceWrapper& deviceWrapper;
	SwapchainWrapper& swapchainWrapper;
	vma::Allocator& allocator;
	vk::DescriptorPool& descPool;
	Lightfield& lightfield;
};
class DisparityRenderpass
{
public:
	DisparityRenderpass() = default;
	~DisparityRenderpass() = default;
	ROF_COPY_MOVE_DELETE(DisparityRenderpass)

public:
	void init(DisparityRenderpassCreateInfo& info)
	{
		//create_shader_modules(deviceWrapper);
		//create_render_pass(deviceWrapper, swapchainWrapper);
		//create_framebuffer(deviceWrapper, swapchainWrapper, input);

		//create_desc_set_layout(deviceWrapper);
		//create_desc_set(deviceWrapper, descPool, input);

		//create_pipeline_layout(deviceWrapper);
		//create_pipeline(deviceWrapper, swapchainWrapper);

		fullscreenRect = vk::Rect2D({ 0, 0 }, info.swapchainWrapper.extent);
	}
	void destroy(DeviceWrapper& deviceWrapper)
	{
		auto& device = deviceWrapper.logicalDevice;

		// Shaders
		device.destroyShaderModule(vs);
		device.destroyShaderModule(ps);

		// Render Pass
		deviceWrapper.logicalDevice.destroyRenderPass(renderPass);

		// Stages
		deviceWrapper.logicalDevice.destroyPipelineLayout(pipelineLayout);
		deviceWrapper.logicalDevice.destroyPipeline(graphicsPipeline);
	}

private:
	static constexpr vk::Format colorFormat = vk::Format::eR8G8B8A8Srgb;
	static constexpr uint32_t nCams = 9;
	vk::RenderPass renderPass;

	// subpasses
	vk::Pipeline graphicsPipeline;
	vk::PipelineLayout pipelineLayout;
	vk::PipelineCache pipelineCache; // TODO

	// shaders for the subpasses
	vk::ShaderModule vs, ps;

	// render resources
	vk::Framebuffer framebuffer;

	// misc
	vk::Rect2D fullscreenRect;
};