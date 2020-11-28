#pragma once

#include "CoreInclude.hpp"

NS_ECS
FORWARD_DEF(NS_VIEW, class PlayerInputMovement);

class ControllerCoordinateSystem
{

public:
	ControllerCoordinateSystem() = default;

	void update(
		f32 deltaTime,
		std::shared_ptr<ecs::view::PlayerInputMovement> view
	);

};

NS_END
