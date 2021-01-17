#pragma once

#include "CoreInclude.hpp"

#include "ecs/view/ECView.hpp"

NS_ECS
NS_VIEW

class PlayerInputMovement : public View
{
	DECLARE_ECS_VIEW_STATICS()
public:
	PlayerInputMovement();
};

NS_END
NS_END
