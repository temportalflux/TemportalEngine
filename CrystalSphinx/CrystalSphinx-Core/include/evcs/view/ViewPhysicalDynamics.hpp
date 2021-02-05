#pragma once

#include "CoreInclude.hpp"

#include "evcs/view/ECView.hpp"

NS_EVCS
NS_VIEW

class PhysicalDynamics : public View
{
	DECLARE_ECS_VIEW_STATICS()
public:
	PhysicalDynamics();
};

NS_END
NS_END
