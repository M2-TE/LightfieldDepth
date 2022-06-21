#pragma once

#include "entities.hpp"
#include "components.hpp"
#include "systems.hpp"

class ECS
{
public:
	ECS() = default;
	~ECS() = default;
	ROF_COPY_MOVE_DELETE(ECS)

public:


private:
	static constexpr uint32_t MAX_ENTITIES = 50;
	static constexpr uint32_t MAX_COMPONENTS = 3;

	std::queue<Entity> availableEntities;
	std::array<ComponentFlags, MAX_ENTITIES> entities;

};