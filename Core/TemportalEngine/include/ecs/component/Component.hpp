#pragma once

#include "ecs/types.h"

#define DECLARE_ECS_COMPONENT_STATICS(POOL_SIZE) \
	public: \
		static ComponentTypeId TypeId; \
		static inline constexpr uSize const MaxPoolSize = POOL_SIZE;
#define DEFINE_ECS_COMPONENT_STATICS(COMP_TYPE) ComponentTypeId COMP_TYPE::TypeId = 0;

NS_ECS
NS_COMPONENT

class Component
{
public:
	// The unique-id for an instance of a given component
	Identifier id;
};

NS_END
NS_END
