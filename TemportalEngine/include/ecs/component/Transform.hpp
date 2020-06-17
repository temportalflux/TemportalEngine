#pragma once

#include "ecs/component/Component.hpp"

#include "math/Vector.hpp"

NS_ECS

struct ComponentTransform : public Component
{
	static ComponentTypeId TypeId;

	ComponentTransform();

	math::Vector3 position;
	math::Quaternion orientation;
	math::Vector3 size;

	ComponentTransform& setPosition(math::Vector3 const &pos);
	ComponentTransform& setOrientation(math::Vector3 const &axis, f32 const &radians);
	void rotate(math::Vector3 const &axis, f32 const &radians);
	ComponentTransform& setSize(math::Vector3 const &size);
};

NS_END
