#pragma once

#include "ecs/ecs.hpp"

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
		ecs.init();

		Entity entity = ecs.create_entity();
		Entity entity2 = ecs.create_entity();
		ecs.destroy_entity(entity);
	}
	void destroy()
	{

	}
	void update()
	{

	}

private:
	ECS ecs;
};