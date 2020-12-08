#include "model/CubeModelLoader.hpp"

Model const& CubeModelLoader::get() const
{
	return this->mModel;
}

CubeModelLoader& CubeModelLoader::pushRight(math::Vector2 const texCoordOffset, math::Vector2 const texCoordSize)
{
	this->pushFace(
		/*U = Front -> Back*/ math::Vector3::ZERO, -math::V3_FORWARD,
		/*V = Top -> Bottom*/ math::V3_UP, math::Vector3::ZERO,
		math::V3_RIGHT,
		texCoordOffset, texCoordSize
	);
	return *this;
}

CubeModelLoader& CubeModelLoader::pushLeft(math::Vector2 const texCoordOffset, math::Vector2 const texCoordSize)
{
	this->pushFace(
		/*U = Back -> Front*/ -math::V3_FORWARD, math::Vector3::ZERO,
		/*V = Top -> Bottom*/ math::V3_UP, math::Vector3::ZERO,
		math::Vector3::ZERO,
		texCoordOffset, texCoordSize
	);
	return *this;
}

CubeModelLoader& CubeModelLoader::pushFront(math::Vector2 const texCoordOffset, math::Vector2 const texCoordSize)
{
	this->pushFace(
		/*U = Left -> Right*/ math::Vector3::ZERO, math::V3_RIGHT,
		/*V = Top -> Bottom*/ math::V3_UP, math::Vector3::ZERO,
		math::Vector3::ZERO,
		texCoordOffset, texCoordSize
	);
	return *this;
}

CubeModelLoader& CubeModelLoader::pushBack(math::Vector2 const texCoordOffset, math::Vector2 const texCoordSize)
{
	this->pushFace(
		/*U = Right -> Left*/ math::V3_RIGHT, math::Vector3::ZERO,
		/*V = Top -> Bottom*/ math::V3_UP, math::Vector3::ZERO,
		-math::V3_FORWARD,
		texCoordOffset, texCoordSize
	);
	return *this;
}

CubeModelLoader& CubeModelLoader::pushUp(math::Vector2 const texCoordOffset, math::Vector2 const texCoordSize)
{
	this->pushFace(
		/*U = Left -> Right*/ math::Vector3::ZERO, math::V3_RIGHT,
		/*V = Front -> Back*/ -math::V3_FORWARD, math::Vector3::ZERO,
		math::V3_UP,
		texCoordOffset, texCoordSize
	);
	return *this;
}

CubeModelLoader& CubeModelLoader::pushDown(math::Vector2 const texCoordOffset, math::Vector2 const texCoordSize)
{
	this->pushFace(
		/*U = Left -> Right*/ math::Vector3::ZERO, math::V3_RIGHT,
		/*V = Back -> Front*/ math::Vector3::ZERO, -math::V3_FORWARD,
		math::Vector3::ZERO,
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
