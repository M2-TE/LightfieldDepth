#pragma once

class RenderPassWrapper
{
public:
	RenderPassWrapper() = default;
	~RenderPassWrapper() = default;
	ROF_COPY_MOVE_DELETE(RenderPassWrapper)

public:

private:
	std::vector<vk::ShaderModule> vertexShaders, pixelShaders;

	std::vector<vk::Pipeline> pipelines;
	std::vector<vk::PipelineLayout> pipelineLayouts;
	std::vector<vk::PipelineCache> pipelineCaches;

	// have this on ring buffer instead?
	// std::vector<vk::Framebuffer> framebuffers;
};