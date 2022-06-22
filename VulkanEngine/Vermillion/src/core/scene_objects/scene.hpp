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

		entity = ecs.create_entity(ComponentFlagBits::eGeometry);
		ecs.destroy_entity(entity);
	}
	void destroy()
	{
		ecs.destroy();
	}
	void update()
	{

	}

private:
	ECS ecs;

	Entity entity; // cube thingy
};