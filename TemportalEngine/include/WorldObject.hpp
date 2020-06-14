#pragma once

#include "TemportalEnginePCH.hpp"

#include "graphics/AttributeBinding.hpp"

#include "glm/glm.hpp"
#include "glm/gtc/quaternion.hpp"

// Objects which exist in the world
// Not all objects need to have a position or render, but most will
// TODO: Give Namespace & move relevant files
class WorldObject
{

public:
	struct InstanceData
	{
		glm::mat4 model;
	};

	WorldObject();

	WorldObject& setPosition(glm::vec3 const &pos);
	WorldObject& setRotation(glm::vec3 const &axis, float const &radians);
	void rotate(glm::vec3 const &axis, float const &radians);
	WorldObject& setScale(glm::vec3 const &size);

	// TODO: Create TemportalEngine math matrix type
	glm::mat4 getModelMatrix() const;

	static std::vector<graphics::AttributeBinding> bindings(ui8 &slot);

private:

	// TODO: Move model matrix stuff (position, rotation, scale) to TransformComponent or ModelComponent
	glm::vec3 mPosition;
	glm::quat mRotation;
	glm::vec3 mScale;
	glm::mat4 mModelMatrix;
	glm::mat4 computeModelMatrix();

};
