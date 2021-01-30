#pragma once

#include "ecs/system/System.hpp"

#include "input/Event.hpp"

NS_ECS
NS_SYSTEM

class IntegratePlayerPhysics : public System
{
public:
	IntegratePlayerPhysics();
	void tick(f32 deltaTime) override;
	void update(f32 deltaTime, ecs::view::View* view) override;
};

NS_END
NS_END
