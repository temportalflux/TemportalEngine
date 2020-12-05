#pragma once

#include "TemportalEnginePCH.hpp"

// The maximum number of entities that can be spawned at once. This defines the pool size for `Entity` instances.
#define ECS_MAX_ENTITY_COUNT 16

// The maximum number of component types that can be registered.
#define ECS_MAX_COMPONENT_TYPE_COUNT 16
// The maximum number of components a given entity may have. This defines how much memory a given `Entity` requires.
#define ECS_ENTITY_MAX_COMPONENT_COUNT 16

// The maximum number of component types that can be registered.
#define ECS_MAX_VIEW_TYPE_COUNT 16
// The maximum number of component slots per `View`.
#define ECS_MAX_COMPONENT_VIEW_SLOTS 16
#define ECS_MAX_VIEWS_PER_ENTITY_COUNT 16
#define ECS_MAX_VIEW_COUNT ECS_MAX_ENTITY_COUNT*ECS_MAX_VIEWS_PER_ENTITY_COUNT

NS_ECS

typedef uIndex ComponentTypeId;
typedef uIndex ViewTypeId;
typedef uIndex Identifier;

NS_END
