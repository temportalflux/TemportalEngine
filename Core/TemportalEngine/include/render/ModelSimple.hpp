#pragma once

#include "TemportalEnginePCH.hpp"

#include "render/ModelVertex.hpp"

NS_RENDER

struct SimpleModel
{
	std::vector<ModelVertex> vertices;
	std::vector<ui32> indices;
};

SimpleModel createIcosphere();

NS_END
