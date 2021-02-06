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
#define ECS_MAX_COMPONENT_VIEW_SLOTS 4
#define ECS_MAX_VIEWS_PER_ENTITY_COUNT 16
#define ECS_MAX_VIEW_COUNT ECS_MAX_ENTITY_COUNT*ECS_MAX_VIEWS_PER_ENTITY_COUNT

NS_EVCS

typedef uIndex TypeId;
typedef TypeId ComponentTypeId;
typedef TypeId ViewTypeId;
typedef uIndex Identifier;

enum class EType : ui8
{
	eEntity = 0,
	eView = 1,
	eComponent = 2,
	eSystem = 3,
};

NS_END