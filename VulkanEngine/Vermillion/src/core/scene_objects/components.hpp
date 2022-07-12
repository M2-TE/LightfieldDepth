#pragma once

#include "utils/types.hpp"
#include "vk_mem_alloc.hpp"
#include "buffers/ring_buffer.hpp"
#include "geometry/vertex.hpp"

#define Component_Flag(comp) typeid(comp).hash_code()

namespace Components
{
	struct Allocator {  };
	struct Deallocator {  };

	struct Transform
	{
		float3 position = float3(0.0f, 0.0f, 0.0f);
		Quaternion rotation = glm::identity<Quaternion>();
		float3 scale = float3(1.0f, 1.0f, 1.0f);
	};
	struct TransformBufferDynamic
	{
		struct BufferFrame
		{
			vk::Buffer buffer;
			vma::Allocation alloc;
			vma::AllocationInfo allocInfo;
			vk::DescriptorSet descSet;
		};
		RingBuffer<BufferFrame> bufferFrames;
	};
	struct TransformBufferStatic
	{
		// TODO: could split this up into more components to improve memory access
		// e.g.: have desc set in a separate buffer, such that many desc sets fit into a single cache line
		vk::Buffer buffer;
		vma::Allocation alloc;
		vma::AllocationInfo allocInfo;
		vk::DescriptorSet descSet;
	};

	enum class Primitive { eCube, eSphere };
	struct Geometry
	{
		Geometry(Primitive primitive)
		{
			switch (primitive) {
				case Primitive::eCube: set_cube(); break;
				case Primitive::eSphere: set_sphere(); break;
			}
		}

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

		vk::Buffer buffer;
		vma::Allocation alloc;

		std::vector<Vertex> vertices;
		std::vector<Index> indices;
	};

	struct Camera
	{
		float aspectRatio = 1280.0f / 720.0f;
		float fov = 45.0f;
		float near = 0.1f, far = 1000.0f;
	};
}