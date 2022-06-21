#pragma once

#include "ecs/manager.hpp"

class Scene
{
public:
	Scene() = default;
	~Scene() = default;
	ROF_COPY_MOVE_DELETE(Scene)

public:
	void init()
	{

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