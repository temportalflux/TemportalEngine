#pragma once

#include "TemportalEnginePCH.hpp"

#include <glm/glm.hpp>
#include "glm/gtc/quaternion.hpp"

class Camera
{

public:
	Camera()
	{
		this->mRotation = (glm::quat)glm::mat4(1);
	}
	
	void move(glm::vec3 amount)
	{
		this->mPosition += amount;
	}

	void rotate(glm::vec3 axis, f32 radians)
	{
		this->mRotation = glm::rotate(this->mRotation, radians, axis);
	}

	glm::vec3 position() const { return this->mPosition; }
	glm::quat rotation() const { return this->mRotation; }
	glm::vec3 forward() const
	{
		static glm::vec4 globalForward = glm::vec4(0, 0, -1, 0);
		return globalForward * this->mRotation;
	}
	glm::vec3 backward() const { return -this->forward(); }
	glm::vec3 right() const
	{
		static glm::vec4 globalRight = glm::vec4(1, 0, 0, 0);
		return globalRight * this->mRotation;
	}
	glm::vec3 left() const { return -this->forward(); }
	glm::vec3 up() const
	{
		static glm::vec4 globalUp = glm::vec4(0, 1, 0, 0);
		return globalUp;
	}
	glm::vec3 down() const { return -this->up(); }

private:
	glm::vec3 mPosition;
	glm::quat mRotation;
};
