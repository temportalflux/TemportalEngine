#pragma once

#include "CoreInclude.hpp"

FORWARD_DEF(NS_ECS, class CoordinateTransform);
FORWARD_DEF(NS_ECS, class PlayerInput);

NS_ECS

class ControllerCoordinateSystem
{

public:
	ControllerCoordinateSystem() = default;

	void update(
		f32 deltaTime,
		std::shared_ptr<ecs::CoordinateTransform> transform,
		std::shared_ptr<ecs::PlayerInput> input
	);

};

NS_END
