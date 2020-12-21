#pragma once

#include "TemportalEnginePCH.hpp"

#include "render/IModel.hpp"
#include "render/ModelVertex.hpp"

NS_RENDER

struct SimpleModel : public IModel<ModelVertex>
{
};

void createIcosahedronFaces(std::vector<math::Vector3> &points, std::vector<math::Vector3UInt> &tris);

SimpleModel createIcosphere(ui8 subdivisions);
SimpleModel createCube();

NS_END
