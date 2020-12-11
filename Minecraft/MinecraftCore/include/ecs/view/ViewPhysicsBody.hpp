#pragma once

#include "CoreInclude.hpp"

#include "ecs/view/ECView.hpp"

NS_ECS
NS_VIEW

class PhysicsBody : public View
{
	DECLARE_ECS_VIEW_STATICS()
public:
	PhysicsBody();
};

NS_END
NS_END
