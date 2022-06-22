#pragma once

#include "entities.hpp"
#include "components.hpp"
#include "systems.hpp"

static constexpr uint32_t nMaxEntities = 5;

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
	}

public:
	Entity create_entity()
	{
		// entity index
		Entity entity = *availableEntityIndices.begin();
		availableEntityIndices.erase(availableEntityIndices.begin());

		return entity;
	}
	Entity create_entity(ComponentFlags components)
	{
		// entity index
		Entity entity = *availableEntityIndices.begin();
		availableEntityIndices.erase(availableEntityIndices.begin());

		// add component flags
		entities[entity] = components;

		return entity;
	}
	void destroy_entity(Entity entity)
	{
		deallocationQueue.push(entity);
	}

	// TODO: allocationQueue.push(entity);

private:
	// each entity is basically an index
	std::set<Entity> availableEntityIndices;
	std::queue<Entity> allocationQueue, deallocationQueue;

	std::array<ComponentFlags, nMaxEntities> entities;
	std::array<ComponentArrayBase*, nMaxComponents> componentArrays;
};