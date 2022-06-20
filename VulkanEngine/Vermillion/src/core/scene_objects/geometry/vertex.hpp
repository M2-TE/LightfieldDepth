#pragma once

struct Vertex
{
public:
	Vertex(float4 pos, float4 col) : pos(pos), col(col) {}

public:
	static vk::VertexInputBindingDescription get_binding_desc() {
		vk::VertexInputBindingDescription desc = vk::VertexInputBindingDescription()
			.setBinding(0)
			.setStride(sizeof(Vertex))
			.setInputRate(vk::VertexInputRate::eVertex);
		return desc;
	}
	static std::array<vk::VertexInputAttributeDescription, 2> get_attr_desc() {
		std::array<vk::VertexInputAttributeDescription, 2> desc;
		desc[0] = vk::VertexInputAttributeDescription()
			.setBinding(0)
			.setLocation(0)
			.setFormat(vk::Format::eR32G32B32A32Sfloat)
			.setOffset(offsetof(Vertex, pos));
		desc[1] = vk::VertexInputAttributeDescription()
			.setBinding(0)
			.setLocation(1)
			.setFormat(vk::Format::eR32G32B32A32Sfloat)
			.setOffset(offsetof(Vertex, col));
		return desc;
	}

public:
	glm::vec<4, glm::f32, glm::packed_highp> pos;
	glm::vec<4, glm::f32, glm::packed_highp> col;
};