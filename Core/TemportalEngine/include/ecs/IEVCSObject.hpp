#pragma once

#include "ecs/types.h"

NS_ECS

class IEVCSObject
{
public:
	virtual ~IEVCSObject() {}
	// The unique-id for an instance of a given component
	Identifier id;
	Identifier netId;
};

NS_END
