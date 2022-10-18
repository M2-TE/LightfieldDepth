#pragma once

#include "components.hpp"
#include "ecs/transform.hpp"
#include "ecs/geometry.hpp"

class Scene
{
public:
	Scene() = default;
	~Scene() = default;
	ROF_COPY_MOVE_DELETE(Scene)

public:
	void init()
	{
		VMI_LOG("[Initializing] Scene...");

		cube = reg.create();
		reg.emplace<components::Geometry>(cube, Primitive::eCube);
		reg.emplace<components::Allocator>(cube);
		reg.emplace<components::Transform>(cube);
		reg.emplace<components::TransformBufferStatic>(cube);

		//sphere = reg.create();
		//reg.emplace<components::Geometry>(sphere, Primitive::eSphere);
		//reg.emplace<components::Allocator>(sphere);
		//reg.emplace<components::Transform>(sphere);
		//reg.emplace<components::TransformBufferStatic>(sphere);
	}
	void destroy()
	{
		reg.emplace<components::Deallocator>(cube);
		//reg.emplace<components::Deallocator>(sphere);
	}
	void update()
	{

	}

public:
	entt::registry reg;
private:
	entt::entity cube;
	entt::entity sphere;
};