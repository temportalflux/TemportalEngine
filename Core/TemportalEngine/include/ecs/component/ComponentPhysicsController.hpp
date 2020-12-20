#pragma once

#include "ecs/component/Component.hpp"

#include "physics/PhysicsController.hpp"

FORWARD_DEF(NS_PHYSICS, class Scene);

NS_ECS
NS_COMPONENT

class PhysicsController : public Component
{
	DECLARE_ECS_COMPONENT_STATICS(16)

public:
	PhysicsController();
	PhysicsController(PhysicsController const& other) = delete;
	~PhysicsController();

	physics::Controller& controller();

private:
	physics::Controller mController;

};

NS_END
NS_END
