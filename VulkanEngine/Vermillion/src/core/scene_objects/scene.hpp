#pragma once

#include "components.hpp"

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

		// TODO: transform component
		cube = reg.create();
		reg.emplace<Components::Geometry>(cube, Components::Primitive::eSphere);
		reg.emplace<Components::Allocator>(cube);

		sphere = reg.create();
		reg.emplace<Components::Geometry>(sphere, Components::Primitive::eSphere);
		reg.emplace<Components::Allocator>(sphere);
	}
	void destroy()
	{
		reg.emplace<Components::Deallocator>(cube);
		reg.emplace<Components::Deallocator>(sphere);
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