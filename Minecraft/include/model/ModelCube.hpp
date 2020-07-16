#pragma once

#include "Model.hpp"

class ModelCube : public Model
{
public:
	ModelCube();

private:
	void pushFace(math::Vector3 uNeg, math::Vector3 uPos, math::Vector3 vNeg, math::Vector3 vPos);

};
