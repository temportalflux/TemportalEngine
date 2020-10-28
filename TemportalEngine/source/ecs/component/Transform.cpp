#include "ecs/component/Transform.hpp"

#include "math/transform.hpp"

using namespace ecs;

ComponentTypeId ComponentTransform::TypeId = 0;

ComponentTransform::ComponentTransform()
{
	this->position = math::Vector3::ZERO;
	this->orientation = math::Quaternion::Identity;
	this->size = math::Vector3({ 1, 1, 1 });
}

ComponentTransform& ComponentTransform::setPosition(math::Vector3 const &pos)
{
	this->position = pos;
	return *this;
}

void ComponentTransform::move(math::Vector3 const &v)
{
	this->position += v;
}

ComponentTransform& ComponentTransform::setOrientation(math::Vector3 const &axis, f32 const &radians)
{
	this->orientation = math::Quaternion::FromAxisAngle(axis, radians);
	return *this;
}

void ComponentTransform::rotate(math::Vector3 const &axis, f32 const &radians)
{
	this->orientation = math::Quaternion::concat(this->orientation, math::Quaternion::FromAxisAngle(axis, radians));
}

ComponentTransform& ComponentTransform::setSize(math::Vector3 const &size)
{
	this->size = size;
	return *this;
}

math::Matrix4x4 ComponentTransform::calculateView() const
{
	return math::lookAt(this->position, this->position + this->forward(), this->up());
}
