#pragma once

#include "ecs/types.h"

NS_ECS

struct Entity
{
	Identifier id;

	struct ComponentEntry
	{
		ComponentTypeId typeId;
		Identifier componentId;
	};
	///*
	ComponentEntry components[ECS_ENTITY_MAX_COMPONENT_COUNT];
	ui8 componentCount;
	//*/

	// TODO: Need functions for:
	// - inserting a component (should be ordered by typeid then component id)
	// - get component (queries the core)
	// - remove all components when the entity is destroyed

};

NS_END
