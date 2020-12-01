#include "WorldObject.hpp"

#include <glm/gtx/transform.hpp>
#include <vulkan/vulkan.hpp>

WorldObject::WorldObject()
{
	this->mPosition = glm::vec3(0);
	this->mRotation = glm::quat(0, 0, 0, 1);
	this->mScale = glm::vec3(1);
	this->mModelMatrix = this->computeModelMatrix();
}

WorldObject& WorldObject::setPosition(glm::vec3 const &pos)
{
	this->mPosition = pos;
	this->mModelMatrix = this->computeModelMatrix();
	return *this;
}

WorldObject& WorldObject::setRotation(glm::vec3 const &axis, float const &radians)
{
	this->mRotation = glm::rotate(glm::quat(), radians, axis);
	this->mModelMatrix = this->computeModelMatrix();
	return *this;
}

void WorldObject::rotate(glm::vec3 const &axis, float const &radians)
{
	this->mRotation = glm::rotate(this->mRotation, radians, axis);
	this->mModelMatrix = this->computeModelMatrix();
}

WorldObject& WorldObject::setScale(glm::vec3 const &size)
{
	this->mScale = size;
	this->mModelMatrix = this->computeModelMatrix();
	return *this;
}

glm::mat4 WorldObject::computeModelMatrix()
{
	auto model = glm::translate(this->mPosition);
	model *= (glm::mat4)this->mRotation;
	model *= glm::scale(this->mScale);
	return model;
}

glm::mat4 WorldObject::getModelMatrix() const
{
	return this->mModelMatrix;
}
