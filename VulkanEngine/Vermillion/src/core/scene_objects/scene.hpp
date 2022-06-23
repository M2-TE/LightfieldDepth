#pragma once

#include "ecs/ecs.hpp"

class Scene
{
public:
	Scene()
	{
		// could eliminate the need to manually create these, can infer from objects that are created?
		ecs.register_component<Components::Geometry>();
		ecs.register_component<Components::Allocator>();
		ecs.register_component<Components::Deallocator>();
		ecs.register_system<Systems::Geometry::Allocator>();
		ecs.register_system<Systems::Geometry::Deallocator>();
		ecs.register_system<Systems::Geometry::Renderer>();
	}
	~Scene()
	{
		ecs.unregister_component<Components::Geometry>();
		ecs.unregister_component<Components::Allocator>();
		ecs.unregister_component<Components::Deallocator>();
	}
	ROF_COPY_MOVE_DELETE(Scene)

public:
	void init()
	{
		VMI_LOG("[Initializing] Scene...");

		size_t components = Component_Flag(Components::Geometry) | Component_Flag(Components::Allocator);
		entity = ecs.create_entity(components);
	}
	void destroy()
	{
		ecs.add_components(entity, Component_Flag(Components::Deallocator));
	}
	void update()
	{

	}


	inline ECS& get_ecs() { return ecs; }

private:
	ECS ecs;

	Entity entity; // cube thingy
};