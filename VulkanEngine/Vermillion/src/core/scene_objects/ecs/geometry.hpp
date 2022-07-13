#pragma once

enum class Primitive { eCube, eSphere };

struct Vertex
{
public:
	Vertex(float4 pos, float4 col, float4 norm) : pos(pos), col(col), norm(norm) {}

public:
	static vk::VertexInputBindingDescription get_binding_desc()
	{
		vk::VertexInputBindingDescription desc = vk::VertexInputBindingDescription()
			.setBinding(0)
			.setStride(sizeof(Vertex))
			.setInputRate(vk::VertexInputRate::eVertex);
		return desc;
	}

	static std::array<vk::VertexInputAttributeDescription, 3> get_attr_desc()
	{
		std::array<vk::VertexInputAttributeDescription, 3> desc;
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
		desc[2] = vk::VertexInputAttributeDescription()
			.setBinding(0)
			.setLocation(2)
			.setFormat(vk::Format::eR32G32B32A32Sfloat)
			.setOffset(offsetof(Vertex, norm));
		return desc;
	}

public:
	float4 pos;
	float4 col;
	float4 norm;
};

namespace components
{
	struct Geometry
	{
	public:
		Geometry(Primitive primitive)
		{
			switch (primitive) {
			case Primitive::eCube: set_cube(); break;
			case Primitive::eSphere: set_sphere(); break;
			}
		}

	private:
		void set_cube()
		{
			const float p = 1.0f, n = -1.0f, z = 0.0f;
			const float4 white = float4(1.0f, 1.0f, 1.0f, 1.0f);
			const float4 red = float4(1.0f, 0.0f, 0.0f, 1.0f);
			const float4 green = float4(0.0f, 1.0f, 0.0f, 1.0f);
			const float4 blue = float4(0.0f, 0.0f, 1.0f, 1.0f);

			// todo: calc the vertices using rotations of a single surface?
			vertices = {
				// right
				Vertex(float4(p, n, n, 1.0f), white, float4(p, z, z, z)),
				Vertex(float4(p, n, p, 1.0f), white, float4(p, z, z, z)),
				Vertex(float4(p, p, n, 1.0f), red  , float4(p, z, z, z)),
				Vertex(float4(p, p, p, 1.0f), red  , float4(p, z, z, z)),

				// left
				Vertex(float4(n, n, n, 1.0f), white, float4(n, z, z, z)),
				Vertex(float4(n, p, n, 1.0f), white, float4(n, z, z, z)),
				Vertex(float4(n, n, p, 1.0f), red  , float4(n, z, z, z)),
				Vertex(float4(n, p, p, 1.0f), red  , float4(n, z, z, z)),

				// bottom
				Vertex(float4(n, p, n, 1.0f), white, float4(z, p, z, z)),
				Vertex(float4(p, p, n, 1.0f), white, float4(z, p, z, z)),
				Vertex(float4(n, p, p, 1.0f), red  , float4(z, p, z, z)),
				Vertex(float4(p, p, p, 1.0f), red  , float4(z, p, z, z)),

				// top
				Vertex(float4(n, n, n, 1.0f), white, float4(z, n, z, z)),
				Vertex(float4(n, n, p, 1.0f), white, float4(z, n, z, z)),
				Vertex(float4(p, n, n, 1.0f), red  , float4(z, n, z, z)),
				Vertex(float4(p, n, p, 1.0f), red  , float4(z, n, z, z)),

				// forward
				Vertex(float4(n, n, p, 1.0f), white, float4(z, z, p, z)),
				Vertex(float4(n, p, p, 1.0f), white, float4(z, z, p, z)),
				Vertex(float4(p, n, p, 1.0f), red  , float4(z, z, p, z)),
				Vertex(float4(p, p, p, 1.0f), red  , float4(z, z, p, z)),

				// backward
				Vertex(float4(n, n, n, 1.0f), white, float4(z, z, n, z)),
				Vertex(float4(p, n, n, 1.0f), white, float4(z, z, n, z)),
				Vertex(float4(n, p, n, 1.0f), red  , float4(z, z, n, z)),
				Vertex(float4(p, p, n, 1.0f), red  , float4(z, z, n, z)),
			};

			indices = {
			0,  1,  2,  2,  1,  3,
			4,  5,  6,  6,  5,  7,
			8,  9,  10, 10, 9,  11,
			12, 13, 14, 14, 13, 15,
			16, 17, 18, 18, 17, 19,
			20, 21, 22, 22, 21, 23,
			};
		}
		void set_sphere()
		{
			static constexpr float4 white = float4(1.0f, 1.0f, 1.0f, 1.0f);
			static constexpr float M = 50.0f;
			static constexpr float N = 50.0f;
			static constexpr float pi = M_PI;

			// TODO: reserve space for vertices/indices?
			for (float m = 0; m <= M; m++) {
				for (float n = 0; n <= N; n++) {
					float x = sinf(pi * m / M) * cosf(2 * pi * n / N);
					float y = sinf(pi * m / M) * sinf(2 * pi * n / N);
					float z = cosf(pi * m / M);

					vertices.emplace_back(float4(x, y, z, 1.0f), white, float4(x, y, z, 1.0f));
				}
			}

			uint nLati = static_cast<uint>(M);
			uint nLongi = static_cast<uint>(N);
			uint nLatiP = nLati + 1u;
			for (uint lati = 0u; lati <= nLati; ++lati) {
				for (uint longi = 0u; longi < nLongi; ++longi) {

					uint latiIndex = lati * nLatiP;
					uint latiIndexP = (lati + 1) * nLatiP;
					uint longIndex = longi;
					uint longIndexP = longi + 1;

					indices.push_back(latiIndex + longIndex);
					indices.push_back(latiIndex + longIndexP);
					indices.push_back(latiIndexP + longIndex);

					indices.push_back(latiIndex + longIndexP);
					indices.push_back(latiIndexP + longIndexP);
					indices.push_back(latiIndexP + longIndex);
				}
			}
		}

	public:
		vk::Buffer buffer;
		vma::Allocation alloc;

		std::vector<Vertex> vertices;
		std::vector<Index> indices;
	};
}

namespace systems
{
	struct Geometry
	{
		static inline void allocate(entt::registry& reg, DeviceWrapper& deviceWrapper, vma::Allocator& allocator, vk::CommandPool& commandPool)
		{
			reg.view<components::Geometry, components::Allocator>().each([&](auto entity, auto& geometry) {
				size_t vertexSize = geometry.vertices.size() * sizeof(Vertex);
				size_t indexSize = geometry.indices.size() * sizeof(Index);
				size_t bufferSize = vertexSize + indexSize;

				// buffer
				vk::BufferCreateInfo bufferInfo = vk::BufferCreateInfo()
					.setSize(bufferSize)
					.setUsage(vk::BufferUsageFlagBits::eTransferDst |
						vk::BufferUsageFlagBits::eVertexBuffer |
						vk::BufferUsageFlagBits::eIndexBuffer);
				vma::AllocationCreateInfo allocCreateInfo = vma::AllocationCreateInfo()
					.setUsage(vma::MemoryUsage::eAutoPreferDevice);
				allocator.createBuffer(&bufferInfo, &allocCreateInfo, &geometry.buffer, &geometry.alloc, nullptr);

				// staging buffer
				bufferInfo = vk::BufferCreateInfo()
					.setSize(bufferSize)
					.setUsage(vk::BufferUsageFlagBits::eTransferSrc);
				allocCreateInfo = vma::AllocationCreateInfo()
					.setUsage(vma::MemoryUsage::eAuto)
					.setFlags(vma::AllocationCreateFlagBits::eHostAccessSequentialWrite | vma::AllocationCreateFlagBits::eMapped);
				vma::AllocationInfo allocInfo;
				auto stagingBuffer = allocator.createBuffer(bufferInfo, allocCreateInfo, allocInfo);

				// already mapped, so just copy over
				memcpy(allocInfo.pMappedData, geometry.vertices.data(), vertexSize);
				memcpy(static_cast<char*>(allocInfo.pMappedData) + vertexSize, geometry.indices.data(), indexSize);

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

					vk::BufferCopy copyRegion = vk::BufferCopy()
						.setSrcOffset(0)
						.setDstOffset(0)
						.setSize(bufferSize);
					commandBuffer.copyBuffer(stagingBuffer.first, geometry.buffer, copyRegion);
					commandBuffer.end();

					vk::SubmitInfo submitInfo = vk::SubmitInfo()
						.setCommandBufferCount(1)
						.setPCommandBuffers(&commandBuffer);
					deviceWrapper.queue.submit(submitInfo);
					deviceWrapper.queue.waitIdle(); // TODO: change this to wait on a fence instead (upon queue submit) so multiple memory transfers would be possible

					// free command buffer directly after use
					deviceWrapper.logicalDevice.freeCommandBuffers(commandPool, commandBuffer);
				}

				allocator.destroyBuffer(stagingBuffer.first, stagingBuffer.second);
			});

			auto view = reg.view<components::Allocator>();
			reg.erase<components::Allocator>(view.begin(), view.end());
		}
		static inline void deallocate(entt::registry& reg, vma::Allocator& allocator)
		{
			reg.view<components::Geometry, components::Deallocator>().each([&](auto entity, auto& geometry) {
				allocator.destroyBuffer(geometry.buffer, geometry.alloc);
			});

			auto view = reg.view<components::Deallocator>();
			reg.destroy(view.begin(), view.end());
		}
		static inline void bind(entt::registry& reg, vk::CommandBuffer& commandBuffer)
		{
			// draw geometry (TODO: use group instead of view when >1 components)
			reg.view<components::Geometry>().each([&](auto& geometry) {
				vk::DeviceSize offsets[] = { 0 };
				commandBuffer.bindVertexBuffers(0, 1, &geometry.buffer, offsets);
				commandBuffer.bindIndexBuffer(geometry.buffer, sizeof(Vertex) * geometry.vertices.size(), vk::IndexType::eUint32);
				commandBuffer.drawIndexed((uint32_t)geometry.indices.size(), 1, 0, 0, 0);
			});
		}
	};
}