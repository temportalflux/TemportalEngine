#pragma once

#include "ecs/system/System.hpp"

NS_EVCS
NS_SYSTEM

class PhysicsIntegration : public System
{
public:
	PhysicsIntegration();
	void update(f32 deltaTime, view::View* view) override;
};

NS_END
NS_END
