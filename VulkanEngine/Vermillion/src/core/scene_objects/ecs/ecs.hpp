#pragma once

static constexpr uint32_t nMaxEntities = 5;
static constexpr uint32_t nMaxComponents = 5;

#include "entities.hpp"
#include "components.hpp"
#include "systems.hpp"

class ECS
{
public:
	ECS()
	{
		// set all entity IDs to available
		for (uint32_t i = 0; i < nMaxEntities; i++) {
			availableEntityIndices.insert(i);
		}
	}
	~ECS() = default;
	ROF_COPY_MOVE_DELETE(ECS)

public:
	Entity create_entity(size_t components)
	{
		// entity index
		Entity entity = *availableEntityIndices.begin();
		availableEntityIndices.erase(availableEntityIndices.begin());

		// assign component flags
		entities[entity] = components;

		// add entity to the system's entity tracker
		for (auto& it : systemEntities) {
			if ((it.first & components) == it.first) {
				it.second.insert(entity);
			}
		}

		return entity;
	}
	void destroy_entity(Entity entity)
	{
		throw std::exception();
		// TODO
	}

	void add_components(Entity entity, size_t components)
	{
		components = entities[entity] | components;
		entities[entity] = components;

		for (auto& it : systemEntities) {
			// add entity to the system's entity tracker
			if ((it.first & components) == it.first) it.second.insert(entity);
		}
	}

	template<typename SystemType, typename... Args> void execute_system(Args... args)
	{
		SystemType::execute(components, systemEntities, args...);
	}

public:
	template<typename T> void unregister_component()
	{
		auto* p = components[typeid(T).hash_code()];
		delete static_cast<Components::ComponentArray<T>*>(p);
	}
	template<typename T> void register_component()
	{
		using namespace Components;
		auto* pBase = static_cast<ComponentArrayBase*>(new ComponentArray<T>());
		components.insert({ typeid(T).hash_code(), pBase });
	}
	template<typename T, typename... Args> void register_system(Args... args)
	{
		using namespace Systems;
		systemEntities.insert({ T().componentFlags, {args...}});
	}
private:

	// TODO
	template<typename T>
	void create()
	{
		VMI_LOG(typeid(T).name());
	}
	template<typename...> struct typelist {};
	void call(typelist<>) { }
	template<typename T, typename ... Rest>
	void call(typelist<T, Rest...>)
	{
		create<T>();
		call(typelist<Rest...>());
	};
	template<typename...classes>
	void createObject() {
		call(typelist<classes...>());
	}


private:
	// each entity is basically an index
	std::set<Entity> availableEntityIndices;
	std::array<size_t, nMaxEntities> entities;

public:
	// ecs data
	// indexed with component hash
	std::unordered_map<size_t, Components::ComponentArrayBase*> components;
	// indexed with system hash
	std::unordered_map<size_t, std::set<Entity>> systemEntities; // contains lists of entities that belong to that specific system
};