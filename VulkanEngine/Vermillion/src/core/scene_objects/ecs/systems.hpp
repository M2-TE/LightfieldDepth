#pragma once

//#include <core/scene_objects/ecs/components.hpp>

namespace Systems
{
#define EXECUTION_INPUT std::unordered_map<size_t, Components::ComponentArrayBase*>& allComponents, std::unordered_map<size_t, std::set<Entity>>& allEntities
#define EXECUTE(...) static inline void execute(EXECUTION_INPUT, __VA_ARGS__)
#define GET_COMPONENTS(name) static_cast<Components::ComponentArray<name>*>(allComponents[Component_Flag(name)])->components
#define GET_ENTITIES(name) allEntities[name().componentFlags];

	struct Geometry
	{
		struct Allocator
		{
			EXECUTE(DeviceWrapper& deviceWrapper, vma::Allocator& allocator, vk::CommandPool& commandPool)
			{
				auto& entities = GET_ENTITIES(Allocator);
				auto& arr = GET_COMPONENTS(Components::Geometry);

				auto cur = entities.cbegin();
				auto end = entities.cend();
				for (; cur != end; cur++) {
					VMI_LOG("Executing Allocator");
				}

				entities.clear();
			}

			const size_t componentFlags = Component_Flag(Components::Geometry) | Component_Flag(Components::Allocator);
		};

		struct Deallocator
		{
			EXECUTE(DeviceWrapper& deviceWrapper)
			{

			}

			const size_t componentFlags = Component_Flag(Components::Geometry) | Component_Flag(Components::Deallocator);
		};

		struct Renderer
		{
			EXECUTE()
			{

			}

			const size_t componentFlags = Component_Flag(Components::Geometry);
		};
	};
}