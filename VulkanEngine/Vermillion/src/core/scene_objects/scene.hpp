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

		cube = reg.create();
		reg.emplace<Components::Geometry>(cube);
		reg.emplace<Components::Allocator>(cube);
	}
	void destroy()
	{
		reg.emplace<Components::Deallocator>(cube);
	}
	void update()
	{

	}

public:
	entt::registry reg;
private:
	entt::entity cube;
};