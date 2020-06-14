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

std::vector<graphics::AttributeBinding> WorldObject::bindings()
{
	return {
		graphics::AttributeBinding(graphics::AttributeBinding::Rate::eVertex)
		.setStructType<VertexData>()
		.addAttribute({ /*slot*/ 0, /*vec2*/ (ui32)vk::Format::eR32G32Sfloat, offsetof(VertexData, position) })
		.addAttribute({ /*slot*/ 1, /*vec3*/ (ui32)vk::Format::eR32G32B32Sfloat, offsetof(VertexData, color) })
	};
}

ui16 WorldObject::pushVertex(VertexData v)
{
	auto i = (ui16)this->mVertices.size();
	this->mVertices.push_back(v);
	return i;
}

void WorldObject::pushIndex(ui16 i)
{
	this->mIndicies.push_back(i);
}

uSize WorldObject::getVertexBufferSize() const
{
	return sizeof(VertexData) * (uSize)this->mVertices.size();
}

uSize WorldObject::getIndexBufferSize() const
{
	return sizeof(ui16) * (uSize)this->mIndicies.size();
}

std::vector<WorldObject::VertexData> WorldObject::verticies() const
{
	return this->mVertices;
}

std::vector<ui16> WorldObject::indicies() const
{
	return this->mIndicies;
}
