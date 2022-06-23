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

		// could eliminate the need to manually create these, can infer from objects that are created?
		ecs.register_component<Components::Geometry>();
		ecs.register_component<Components::Allocator>();
		ecs.register_component<Components::Deallocator>();
		ecs.register_system<Systems::Geometry::Allocator>();
		ecs.register_system<Systems::Geometry::Deallocator>();
		ecs.register_system<Systems::Geometry::Renderer>();

		size_t components = Component_Flag(Components::Geometry) | Component_Flag(Components::Allocator);
		entity = ecs.create_entity(components);

		//createObject<Scene, Input>();
	}
	void destroy()
	{
		ecs.destroy();
	}
	void update()
	{

	}


	inline ECS& get_ecs() { return ecs; }

private:
	ECS ecs;

	Entity entity; // cube thingy
};