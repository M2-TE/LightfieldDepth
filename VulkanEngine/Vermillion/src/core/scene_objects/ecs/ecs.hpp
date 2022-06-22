#pragma once

#include "entities.hpp"
#include "components.hpp"
#include "systems.hpp"

static constexpr uint32_t nMaxEntities = 5;
static constexpr uint32_t nMaxComponents = 5;

// represents an array of components to index into using entity ids
struct ComponentArrayBase {};
template<class T> struct ComponentArray : public ComponentArrayBase { std::array<T, nMaxEntities> components; };

class ECS
{
public:
	ECS() = default;
	~ECS() = default;
	ROF_COPY_MOVE_DELETE(ECS)

public:
	void init()
	{
		// set all entity IDs to available
		for (uint32_t i = 0; i < nMaxEntities; i++) {
			availableEntityIndices.insert(i);
		}

		add_system<Allocator>();
	}
	void destroy()
	{
		del_system<Deallocator>();
	}

public:
	Entity create_entity(ComponentFlags components)
	{
		//// entity index
		//Entity entity = *availableEntityIndices.begin();
		//availableEntityIndices.erase(availableEntityIndices.begin());

		//// add component flags
		//entities[entity] = components;

		//return entity;
		return 0;
	}
	void destroy_entity(Entity entity)
	{
		//deallocationQueue.push(entity);
	}

private:
	template<class T> void add_system()
	{
		auto* pBase = static_cast<SystemBase*>(new T());
		systems.insert({ typeid(T).hash_code(), pBase });
	}
	template<class T> void add_component()
	{
		auto* pBase = static_cast<ComponentArrayBase*>(new ComponentArray<T>());
		components.insert({ typeid(ComponentArray<T>).hash_code(), pBase });
	}
	template<class T> void del_system()
	{
		auto* pBase = systems[typeid(T).hash_code()];
		delete static_cast<T*>(pBase);
	}
	template<class T> void del_component()
	{
		auto* pBase = components[typeid(ComponentArray<T>).hash_code()];
		delete static_cast<ComponentArray<T>*>(pBase);
	}

private:
	// each entity is basically an index
	std::set<Entity> availableEntityIndices;

	// ecs data
	std::array<ComponentFlags, nMaxEntities> entities;
	std::unordered_map<size_t, ComponentArrayBase*> components;
	std::unordered_map<size_t, SystemBase*> systems;
};