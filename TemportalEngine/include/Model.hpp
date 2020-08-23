#pragma once

#include "TemportalEnginePCH.hpp"

#include "graphics/AttributeBinding.hpp"
#include "math/Vector.hpp"

class Model
{

public:
	struct Vertex
	{
		math::Vector3Padded position;
		math::Vector4 color;
		math::Vector2Padded texCoord;
	};

	static std::vector<graphics::AttributeBinding> bindings(ui8 &slot);

	ui16 pushVertex(Vertex v);
	ui16 pushVertex(math::Vector3 pos, math::Vector2 texCoord);
	void pushIndex(ui16 i);

	uSize getVertexBufferSize() const;
	uSize getIndexBufferSize() const;
	std::vector<Vertex> verticies() const;
	std::vector<ui16> indicies() const;

private:

	std::vector<Vertex> mVertices;
	std::vector<ui16> mIndicies;

};
