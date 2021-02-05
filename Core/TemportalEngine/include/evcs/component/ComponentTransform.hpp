#pragma once

#include "evcs/component/Component.hpp"

#include "math/Matrix.hpp"

NS_EVCS
NS_COMPONENT

class Transform : public Component
{
	DECLARE_ECS_COMPONENT_STATICS(0)

public:
	Transform();

	math::Vector3 position;
	math::Quaternion orientation;
	math::Vector3 size;

	math::Vector3 forward() const
	{
		return this->orientation.rotate(math::Vector3unitZ);
	}
	math::Vector3 backward() const
	{
		return this->orientation.rotate(-math::Vector3unitZ);
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
		return this->orientation.rotate(math::Vector3unitY);
	}
	math::Vector3 down() const { return -this->up(); }

	Transform& setPosition(math::Vector3 const &pos);
	void move(math::Vector3 const &v);
	Transform& setOrientation(math::Vector3 const &axis, f32 const &radians);
	void rotate(math::Vector3 const &axis, f32 const &radians);
	Transform& setSize(math::Vector3 const &size);

	math::Matrix4x4 calculateView() const;
	math::Matrix4x4 calculateViewFrom(math::Vector3 const &pos) const;
	
};

NS_END
NS_END
