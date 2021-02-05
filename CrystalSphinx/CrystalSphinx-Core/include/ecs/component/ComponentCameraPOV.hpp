#pragma once

#include "ecs/component/Component.hpp"

#include "input/Event.hpp"

NS_EVCS
NS_COMPONENT

class CameraPOV : public Component
{
	DECLARE_ECS_COMPONENT_STATICS(4)

public:
	CameraPOV();

	/**
	 * Sets the Vertical FOV of the camera (which is important when considering ultrawide monitors).
	 * Here is a calculator to go from horizontal to vertical: http://themetalmuncher.github.io/fov-calc/
	 */
	CameraPOV& setFOV(f32 verticalFOV);

	f32 const& fov() const;
	f32 const& nearPlane() const;
	f32 const& farPlane() const;

private:
	/**
	 * This is the vertical FOV of the camera.
	 * Fun Math Fact: the equation is almost the same from h->v as v->h:
	 * v = 2 * atan(tan(h / 2) * height / width)
	 * h = 2 * atan(tan(v / 2) * width / height)
	 */
	f32 mFOV;
	f32 mNearPlane;
	f32 mFarPlane;

};

NS_END
NS_END
