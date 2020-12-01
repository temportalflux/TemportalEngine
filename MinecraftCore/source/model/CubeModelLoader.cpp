#include "model/CubeModelLoader.hpp"

Model const& CubeModelLoader::get() const
{
	return this->mModel;
}

CubeModelLoader& CubeModelLoader::pushRight(math::Vector2 const texCoordOffset, math::Vector2 const texCoordSize)
{
	// Right +X	<U, V> = <-Z, Y>
	this->pushFace(
		/*U = Z 1->0*/{ 0, 0, 1 }, { 0, 0, 0 },
		/*V = Y 1->0*/{ 0, 1, 0 }, { 0, 0, 0 },
		/*Offset X 1*/{ 1, 0, 0 },
		texCoordOffset, texCoordSize
	);
	return *this;
}

CubeModelLoader& CubeModelLoader::pushLeft(math::Vector2 const texCoordOffset, math::Vector2 const texCoordSize)
{
	// Left -X	<U, V> = <+Z, Y>
	this->pushFace(
		/*U = Z 0->1*/{ 0, 0, 0 }, { 0, 0, 1 },
		/*V = Y 1->0*/{ 0, 1, 0 }, { 0, 0, 0 },
		/*Offset X 0*/{ 0, 0, 0 },
		texCoordOffset, texCoordSize
	);
	return *this;
}

CubeModelLoader& CubeModelLoader::pushFront(math::Vector2 const texCoordOffset, math::Vector2 const texCoordSize)
{
	// Forward +Z	<U, V> = <-X, Y>
	this->pushFace(
		/*U = X 0->1*/{ 0, 0, 0 }, { 1, 0, 0 },
		/*V = Y 1->0*/{ 0, 1, 0 }, { 0, 0, 0 },
		/*Offset Z 0*/{ 0, 0, 1 },
		texCoordOffset, texCoordSize
	);
	return *this;
}

CubeModelLoader& CubeModelLoader::pushBack(math::Vector2 const texCoordOffset, math::Vector2 const texCoordSize)
{
	// Backward -Z	<U, V> = <+X, Y>
	this->pushFace(
		/*U = X 1->0*/{ 1, 0, 0 }, { 0, 0, 0 },
		/*V = Y 1->0*/{ 0, 1, 0 }, { 0, 0, 0 },
		/*Offset Z 0*/{ 0, 0, 0 },
		texCoordOffset, texCoordSize
	);
	return *this;
}

CubeModelLoader& CubeModelLoader::pushUp(math::Vector2 const texCoordOffset, math::Vector2 const texCoordSize)
{
	// Up +Y	<U, V> = <-X, Z>
	this->pushFace(
		/*U = X 0->1*/{ 0, 0, 0 }, { 1, 0, 0 },
		/*V = Z 0->1*/{ 0, 0, 0 }, { 0, 0, 1 },
		/*Offset Y 1*/{ 0, 1, 0 },
		texCoordOffset, texCoordSize
	);
	return *this;
}

CubeModelLoader& CubeModelLoader::pushDown(math::Vector2 const texCoordOffset, math::Vector2 const texCoordSize)
{
	// Down -Y	<U, V> = <+X, Z>
	this->pushFace(
		/*U = X 1->0*/{ 1, 0, 0 }, { 0, 0, 0 },
		/*V = Z 0->1*/{ 0, 0, 0 }, { 0, 0, 1 },
		/*Offset Y 0*/{ 0, 0, 0 },
		texCoordOffset, texCoordSize
	);
	return *this;
}

void CubeModelLoader::pushFace(
	math::Vector3 uNeg, math::Vector3 uPos,
	math::Vector3 vNeg, math::Vector3 vPos, math::Vector3 axis,
	math::Vector2 const texCoordOffset, math::Vector2 const texCoordSize
)
{
	auto idxTL = this->mModel.pushVertex(uNeg + vNeg + axis, texCoordOffset + texCoordSize * math::Vector2({ 0.0f, 0.0f }));
	auto idxTR = this->mModel.pushVertex(uPos + vNeg + axis, texCoordOffset + texCoordSize * math::Vector2({ 1.0f, 0.0f }));
	auto idxBL = this->mModel.pushVertex(uNeg + vPos + axis, texCoordOffset + texCoordSize * math::Vector2({ 0.0f, 1.0f }));
	auto idxBR = this->mModel.pushVertex(uPos + vPos + axis, texCoordOffset + texCoordSize * math::Vector2({ 1.0f, 1.0f }));
	this->mModel.pushIndex(idxTL);
	this->mModel.pushIndex(idxTR);
	this->mModel.pushIndex(idxBR);
	this->mModel.pushIndex(idxBR);
	this->mModel.pushIndex(idxBL);
	this->mModel.pushIndex(idxTL);
}
