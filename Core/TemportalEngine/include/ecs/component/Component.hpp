#pragma once

#include "ecs/IEVCSObject.hpp"

#define DECLARE_ECS_COMPONENT_STATICS(POOL_SIZE) \
	public: \
		static ComponentTypeId TypeId; \
		static inline constexpr uSize const MaxPoolSize = POOL_SIZE;
#define DEFINE_ECS_COMPONENT_STATICS(COMP_TYPE) ComponentTypeId COMP_TYPE::TypeId = 0;

NS_ECS
NS_COMPONENT

class Component : public ecs::IEVCSObject
{
};

NS_END
NS_END
