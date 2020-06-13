#include "WorldObject.hpp"

#include <glm/gtx/transform.hpp>

WorldObject::WorldObject()
{
	this->mPosition = glm::vec3(0);
	this->mRotation = glm::quat();
	this->mScale = glm::vec3(1);
}

WorldObject& WorldObject::setPosition(glm::vec3 const &pos)
{
	this->mPosition = pos;
	return *this;
}

WorldObject& WorldObject::setRotation(glm::vec3 const &axis, float const &radians)
{
	this->mRotation = glm::rotate(glm::quat(), radians, axis);
	return *this;
}

WorldObject& WorldObject::setScale(glm::vec3 const &size)
{
	this->mScale = size;
	return *this;
}

glm::mat4 WorldObject::getModelMatrix() const
{
	// TODO: Find a way to compute model matrix on the fly when any of position, rotation, or scale is changed
	auto model = glm::translate(this->mPosition);
	model *= (glm::mat4)this->mRotation;
	model *= glm::scale(this->mScale);
	return model;
}

ui16 WorldObject::pushVertex(Vertex v)
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
	return sizeof(Vertex) * (uSize)this->mVertices.size();
}

uSize WorldObject::getIndexBufferSize() const
{
	return sizeof(ui16) * (uSize)this->mIndicies.size();
}

std::vector<WorldObject::Vertex> WorldObject::verticies() const
{
	return this->mVertices;
}

std::vector<ui16> WorldObject::indicies() const
{
	return this->mIndicies;
}
