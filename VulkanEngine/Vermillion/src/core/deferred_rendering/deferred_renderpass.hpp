#pragma once

struct DeferredRenderpassCreateInfo
{
	DeviceWrapper& deviceWrapper;
	SwapchainWrapper& swapchainWrapper;
	vma::Allocator& allocator;
	vk::RenderPass& renderPass;
	vk::DescriptorPool& descPool;
	vk::ImageView& output, depthStencil;
	size_t nMaxFrames;
};

#include "gbuffer.hpp"

class DeferredRenderpass
{
public:
	DeferredRenderpass() = default;
	~DeferredRenderpass() = default;
	ROF_COPY_MOVE_DELETE(DeferredRenderpass)

public:
	void init(DeferredRenderpassCreateInfo& info)
	{
		create_shader_modules(info);
		create_desc_set_layout(info);
		for (size_t i = 0; i < info.nMaxFrames; i++) gbuffers[i].init(info, descSetLayout);
		create_framebuffers(info);
	}
	void destroy(DeviceWrapper& deviceWrapper, vma::Allocator& allocator)
	{
		auto& device = deviceWrapper.logicalDevice;

		device.destroyShaderModule(geometryVS);
		device.destroyShaderModule(geometryPS);
		device.destroyShaderModule(lightingVS);
		device.destroyShaderModule(lightingPS);
		device.destroyDescriptorSetLayout(descSetLayout);
		for (size_t i = 0; i < gbuffers.size(); i++) gbuffers[i].destroy(deviceWrapper, allocator);
	}

private:
	void create_shader_modules(DeferredRenderpassCreateInfo& info)
	{
		geometryVS = create_shader_module(info.deviceWrapper, geometry_pass_vs, geometry_pass_vs_size);
		geometryPS = create_shader_module(info.deviceWrapper, geometry_pass_ps, geometry_pass_ps_size);

		lightingVS = create_shader_module(info.deviceWrapper, lighting_pass_vs, lighting_pass_vs_size);
		lightingPS = create_shader_module(info.deviceWrapper, lighting_pass_ps, lighting_pass_ps_size);
	}
	void create_desc_set_layout(DeferredRenderpassCreateInfo& info)
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
		descSetLayout = info.deviceWrapper.logicalDevice.createDescriptorSetLayout(createInfo);
	}
	void create_framebuffers(DeferredRenderpassCreateInfo& info)
	{
		for (size_t i = 0; i < info.nMaxFrames; i++) {
			std::array<vk::ImageView, 5> attachments = {
				info.output,
				info.depthStencil,
				gbuffers[i].posImageView,
				gbuffers[i].colImageView,
				gbuffers[i].normImageView
			};

			vk::FramebufferCreateInfo framebufferInfo = vk::FramebufferCreateInfo()
				.setRenderPass(info.renderPass)
				.setWidth(info.swapchainWrapper.extent.width)
				.setHeight(info.swapchainWrapper.extent.height)
				.setLayers(1)
				// attachments
				.setAttachmentCount(attachments.size()).setPAttachments(attachments.data());

			framebuffers[i] = info.deviceWrapper.logicalDevice.createFramebuffer(framebufferInfo);
		}
	}

private:
	std::vector<GBuffer> gbuffers;
	std::vector<vk::Framebuffer> framebuffers;
	vk::DescriptorSetLayout descSetLayout;

	// subpasses
	vk::Pipeline geometryPassPipeline, lightingPassPipeline;
	vk::PipelineLayout geometryPassPipelineLayout, lightingPassPipelineLayout;
	vk::PipelineCache geometryPassPipelineCache, lightingPassPipelineCache;

	// shaders for the subpasses
	vk::ShaderModule geometryVS, geometryPS;
	vk::ShaderModule lightingVS, lightingPS;
};