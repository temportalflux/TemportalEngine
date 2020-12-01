#include "ecs/component/ComponentTransform.hpp"

#include "math/transform.hpp"

using namespace ecs;
using namespace ecs::component;

DEFINE_ECS_COMPONENT_STATICS(Transform)

Transform::Transform()
{
	this->position = math::Vector3::ZERO;
	this->orientation = math::Quaternion::Identity;
	this->size = math::Vector3({ 1, 1, 1 });
}

Transform& Transform::setPosition(math::Vector3 const &pos)
{
	this->position = pos;
	return *this;
}

void Transform::move(math::Vector3 const &v)
{
	this->position += v;
}

Transform& Transform::setOrientation(math::Vector3 const &axis, f32 const &radians)
{
	this->orientation = math::Quaternion::FromAxisAngle(axis, radians);
	return *this;
}

void Transform::rotate(math::Vector3 const &axis, f32 const &radians)
{
	this->orientation = math::Quaternion::concat(this->orientation, math::Quaternion::FromAxisAngle(axis, radians));
}

Transform& Transform::setSize(math::Vector3 const &size)
{
	this->size = size;
	return *this;
}

math::Matrix4x4 Transform::calculateView() const
{
	return calculateViewFrom(this->position);
}

math::Matrix4x4 Transform::calculateViewFrom(math::Vector3 const &pos) const
{
	OPTICK_EVENT();
	return math::lookAt(pos, pos + this->forward(), this->up());
}
