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

std::vector<graphics::AttributeBinding> WorldObject::bindings(ui8 &slot)
{
	return {
		// Data per object instance - this is only for objects which dont more, rotate, or scale
		graphics::AttributeBinding(graphics::AttributeBinding::Rate::eInstance)
		.setStructType<InstanceData>()
		// mat4 using 4 slots
		.addAttribute({ /*slot*/ slot++, /*mat4*/ (ui32)vk::Format::eR32G32B32A32Sfloat, offsetof(InstanceData, model) + (0 * sizeof(glm::vec4))})
		.addAttribute({ /*slot*/ slot++, /*mat4*/ (ui32)vk::Format::eR32G32B32A32Sfloat, offsetof(InstanceData, model) + (1 * sizeof(glm::vec4))})
		.addAttribute({ /*slot*/ slot++, /*mat4*/ (ui32)vk::Format::eR32G32B32A32Sfloat, offsetof(InstanceData, model) + (2 * sizeof(glm::vec4))})
		.addAttribute({ /*slot*/ slot++, /*mat4*/ (ui32)vk::Format::eR32G32B32A32Sfloat, offsetof(InstanceData, model) + (3 * sizeof(glm::vec4))})
	};
}
