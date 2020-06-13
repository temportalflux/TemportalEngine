#pragma once

#include "TemportalEnginePCH.hpp"

#include "glm/glm.hpp"
#include "glm/gtc/quaternion.hpp"

// Objects which exist in the world
// Not all objects need to have a position or render, but most will
// TODO: Give Namespace & move relevant files
class WorldObject
{

public:

	struct Vertex
	{
		glm::vec2 position;
		glm::vec3 color;
	};

	WorldObject();

	WorldObject& setPosition(glm::vec3 const &pos);
	WorldObject& setRotation(glm::vec3 const &axis, float const &radians);
	WorldObject& setScale(glm::vec3 const &size);

	// TODO: Create you own matrix type
	glm::mat4 getModelMatrix() const;

	ui16 pushVertex(Vertex v);
	void pushIndex(ui16 i);
	uSize getVertexBufferSize() const;
	uSize getIndexBufferSize() const;
	std::vector<Vertex> verticies() const;
	std::vector<ui16> indicies() const;

private:

	// TODO: Move model matrix stuff (position, rotation, scale) to TransformComponent or ModelComponent
	glm::vec3 mPosition;
	glm::quat mRotation;
	glm::vec3 mScale;

	// TODO: Move to model component
	std::vector<Vertex> mVertices;
	std::vector<ui16> mIndicies;

};
