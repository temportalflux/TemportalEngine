#pragma once

#include "CoreInclude.hpp"

#include "ecs/view/ECView.hpp"

NS_EVCS
NS_VIEW

class PlayerPhysics : public View
{
	DECLARE_ECS_VIEW_STATICS()
public:
	PlayerPhysics();
	void onComponentReplicationUpdate(component::Component* component) override;
};

NS_END
NS_END
