#pragma once

#include "ecs/component/Component.hpp"

#include "input/Event.hpp"

NS_ECS
NS_COMPONENT

class CameraPOV : public Component
{
	DECLARE_ECS_COMPONENT_STATICS(4)

public:
	CameraPOV();

	f32 const& fov() const;

private:
	// TODO: this should be a vertical FOV to account for ultrawide monitors
	f32 mFOV;

};

NS_END
NS_END
