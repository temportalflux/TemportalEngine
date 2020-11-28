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
	f32 const& nearPlane() const;
	f32 const& farPlane() const;

private:
	/**
	 * This is the vertical FOV of the camera.
	 * Vertical is used to account for ultrawide monitors.
	 * Here is a calculator to go from horizontal to vertical: http://themetalmuncher.github.io/fov-calc/
	 * The equation is almost the same from h->v as v->h:
	 * v = 2 * atan(tan(h / 2) * height / width)
	 * h = 2 * atan(tan(v / 2) * width / height)
	 */
	f32 mFOV;
	f32 mNearPlane;
	f32 mFarPlane;

};

NS_END
NS_END
