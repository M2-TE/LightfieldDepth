#pragma once

namespace components
{
	struct Allocator {  };
	struct Deallocator {  };

	struct Camera
	{
		float aspectRatio = 1280.0f / 720.0f;
		float fov = 45.0f;
		float near = 0.1f, far = 1000.0f;
	};
}