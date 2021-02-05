#pragma once

#include "evcs/system/System.hpp"

#include "input/Event.hpp"

NS_EVCS
NS_SYSTEM

class IntegratePlayerPhysics : public System
{
public:
	IntegratePlayerPhysics();
	void tick(f32 deltaTime) override;
	void update(f32 deltaTime, view::View* view) override;
};

NS_END
NS_END
