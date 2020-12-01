#pragma once

#include "CoreInclude.hpp"

#include "Model.hpp"

class CubeModelLoader
{

public:

	Model const& get() const;

	CubeModelLoader& pushRight(math::Vector2 const texCoordOffset, math::Vector2 const texCoordSize);
	CubeModelLoader& pushLeft(math::Vector2 const texCoordOffset, math::Vector2 const texCoordSize);
	CubeModelLoader& pushFront(math::Vector2 const texCoordOffset, math::Vector2 const texCoordSize);
	CubeModelLoader& pushBack(math::Vector2 const texCoordOffset, math::Vector2 const texCoordSize);
	CubeModelLoader& pushUp(math::Vector2 const texCoordOffset, math::Vector2 const texCoordSize);
	CubeModelLoader& pushDown(math::Vector2 const texCoordOffset, math::Vector2 const texCoordSize);

private:
	Model mModel;

	void pushFace(
		math::Vector3 uNeg, math::Vector3 uPos,
		math::Vector3 vNeg, math::Vector3 vPos, math::Vector3 axis,
		math::Vector2 texCoordOffset, math::Vector2 texCoordSize
	);

};
