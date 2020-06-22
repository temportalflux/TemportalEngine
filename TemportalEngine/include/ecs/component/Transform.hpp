#pragma once

#include "ecs/component/Component.hpp"

#include "math/Matrix.hpp"

NS_ECS

struct ComponentTransform : public Component
{
	static ComponentTypeId TypeId;

	ComponentTransform();

	math::Vector3 position;
	math::Quaternion orientation;
	math::Vector3 size;

	math::Vector3 forward() const
	{
		return this->orientation.rotate(math::Vector3unitY);
	}
	math::Vector3 backward() const
	{
		return this->orientation.rotate(-math::Vector3unitY);
	}
	math::Vector3 right() const
	{
		return this->orientation.rotate(math::Vector3unitX);
	}
	math::Vector3 left() const
	{
		return this->orientation.rotate(-math::Vector3unitX);
	}
	math::Vector3 up() const
	{
		return this->orientation.rotate(math::Vector3unitZ);
		//return math::Vector3unitZ;
	}
	math::Vector3 down() const { return -this->up(); }

	ComponentTransform& setPosition(math::Vector3 const &pos);
	void move(math::Vector3 const &v);
	ComponentTransform& setOrientation(math::Vector3 const &axis, f32 const &radians);
	void rotate(math::Vector3 const &axis, f32 const &radians);
	ComponentTransform& setSize(math::Vector3 const &size);

	math::Matrix4x4 calculateView() const;
	
};

NS_END
