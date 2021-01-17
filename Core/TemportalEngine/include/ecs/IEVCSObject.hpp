#pragma once

#include "ecs/types.h"

NS_ECS

struct IEVCSObject
{
public:
	// The unique-id for an instance of a given component
	Identifier id;
	Identifier netId;
};

NS_END
