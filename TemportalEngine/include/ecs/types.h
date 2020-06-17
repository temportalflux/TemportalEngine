#pragma once

#include "TemportalEnginePCH.hpp"

#define ECS_MAX_ENTITY_COUNT 1024
#define ECS_MAX_COMPONENT_COUNT 1024
#define ECS_ENTITY_MAX_COMPONENT_COUNT 32
#define ECS_MAX_COMPONENT_TYPE_COUNT 16

NS_ECS

typedef uSize ComponentTypeId;
// TODO: Might be worth considering if guids are really worth it (they are 16 bytes a pop). Might be able to use ui16 instead (max of 65536)
//typedef utility::Guid Identifier;
typedef ui16 Identifier;

struct Entity
{
	Identifier id;
};

NS_END
