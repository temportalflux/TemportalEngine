#pragma once

#include "TemportalEnginePCH.hpp"

// The maximum number of entities that can be spawned at once. This defines the pool size for `Entity` instances.
#define ECS_MAX_ENTITY_COUNT 16
// The maximum number of components a given entity may have. This defines how much memory a given `Entity` requires.
#define ECS_ENTITY_MAX_COMPONENT_COUNT 16
// The maximum number of component types that can be registered.
#define ECS_MAX_COMPONENT_TYPE_COUNT 16

NS_ECS

typedef uIndex ComponentTypeId;
typedef ui16 Identifier;

NS_END
