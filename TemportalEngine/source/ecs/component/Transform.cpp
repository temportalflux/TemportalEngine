#include "ecs/component/Transform.hpp"

using namespace ecs;

ComponentTypeId ComponentTransform::TypeId = 0;

ComponentTransform::ComponentTransform()
{
	this->position = math::Vector3::ZERO;
	this->orientation = math::Vector4unitW;
	this->orientation = math::QuaternionIdentity;
	this->size = math::Vector3({ 1, 1, 1 });
}

ComponentTransform& ComponentTransform::setPosition(math::Vector3 const &pos)
{
	this->position = pos;
	return *this;
}

ComponentTransform& ComponentTransform::setOrientation(math::Vector3 const &axis, f32 const &radians)
{
	this->orientation = math::QuaternionFromAxisAngle(axis, radians);
	return *this;
}

void ComponentTransform::rotate(math::Vector3 const &axis, f32 const &radians)
{
	this->orientation = math::QuaternionConcatenate(this->orientation, math::QuaternionFromAxisAngle(axis, radians));
}

ComponentTransform& ComponentTransform::setSize(math::Vector3 const &size)
{
	this->size = size;
	return *this;
}
