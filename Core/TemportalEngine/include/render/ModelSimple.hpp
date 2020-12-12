#pragma once

#include "TemportalEnginePCH.hpp"

#include "render/ModelVertex.hpp"

NS_RENDER

struct SimpleModel
{
	std::vector<ModelVertex> vertices;
	std::vector<ui32> indices;

	ui32 pushVertex(ModelVertex const& vertex);
	void pushTri(math::Vector3UInt const& tri);
};

void createIcosahedronFaces(std::vector<math::Vector3> &points, std::vector<math::Vector3UInt> &tris);

SimpleModel createIcosphere(ui8 subdivisions);
SimpleModel createCube();

NS_END
