#include "model/ModelCube.hpp"

ModelCube::ModelCube() : Model()
{
	// <x, y, z> = <Right/Left, Forward/Backward, Up/Down>
	// Right +X	<U, V> = <-Y, Z>
	this->pushFace(
		/*U = Y 0->1*/{ 0, 0, 0 }, { 0, 1, 0 },
		/*V = Z 0->1*/{ 0, 0, 0 }, { 0, 0, 1 },
		/*Offset X 1*/{ 1, 0, 0 }
	);
	// Left -X	<U, V> = <+Y, Z>
	this->pushFace(
		/*U = Y 1->0*/{ 0, 1, 0 }, { 0, 0, 0 },
		/*V = Z 0->1*/{ 0, 0, 0 }, { 0, 0, 1 },
		/*Offset X 0*/{ 0, 0, 0 }
	);
	// Forward +Y	<U, V> = <-X, Z>
	this->pushFace(
		/*U = X 0->1*/{ 0, 0, 0 }, { 1, 0, 0 },
		/*V = Z 0->1*/{ 0, 0, 0 }, { 0, 0, 1 },
		/*Offset Y 0*/{ 0, 0, 0 }
	);
	// Backward -Y	<U, V> = <+X, Z>
	this->pushFace(
		/*U = X 1->0*/{ 1, 0, 0 }, { 0, 0, 0 },
		/*V = Z 0->1*/{ 0, 0, 0 }, { 0, 0, 1 },
		/*Offset Y 1*/{ 0, 1, 0 }
	);
	// Up +Z	<U, V> = <-X, Y>
	this->pushFace(
		/*U = X 0->1*/{ 0, 0, 0 }, { 1, 0, 0 },
		/*V = Y 0->1*/{ 0, 0, 0 }, { 0, 1, 0 },
		/*Offset Z 1*/{ 0, 0, 1 }
	);
	// Down -Z	<U, V> = <+X, Y>
	this->pushFace(
		/*U = X 1->0*/{ 1, 0, 0 }, { 0, 0, 0 },
		/*V = Y 0->1*/{ 0, 0, 0 }, { 0, 1, 0 },
		/*Offset Z 0*/{ 0, 0, 0 }
	);
}

void ModelCube::pushFace(math::Vector3 uNeg, math::Vector3 uPos, math::Vector3 vNeg, math::Vector3 vPos, math::Vector3 axis)
{
	auto idxTL = this->pushVertex(uNeg + vNeg + axis, { 0.0f, 0.0f });
	auto idxTR = this->pushVertex(uPos + vNeg + axis, { 1.0f, 0.0f });
	auto idxBL = this->pushVertex(uNeg + vPos + axis, { 0.0f, 1.0f });
	auto idxBR = this->pushVertex(uPos + vPos + axis, { 1.0f, 1.0f });
	this->pushIndex(idxTL);
	this->pushIndex(idxTR);
	this->pushIndex(idxBR);
	this->pushIndex(idxBR);
	this->pushIndex(idxBL);
	this->pushIndex(idxTL);
}
