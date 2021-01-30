#pragma once

#include "CoreInclude.hpp"

#include "ecs/view/ECView.hpp"

NS_ECS
NS_VIEW

class PlayerPhysics : public View
{
	DECLARE_ECS_VIEW_STATICS()
public:
	PlayerPhysics();
};

NS_END
NS_END
