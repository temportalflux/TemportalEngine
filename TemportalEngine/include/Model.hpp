#pragma once

#include "TemportalEnginePCH.hpp"

#include "graphics/AttributeBinding.hpp"

#include "glm/glm.hpp"

class Model
{

public:
	struct Vertex
	{
		glm::vec3 position;
		glm::vec3 color;
	};

	static std::vector<graphics::AttributeBinding> bindings(ui8 &slot);

	uSize getVertexBufferSize() const;
	uSize getIndexBufferSize() const;
	std::vector<Vertex> verticies() const;
	std::vector<ui16> indicies() const;

protected:
	ui16 pushVertex(Vertex v);
	void pushIndex(ui16 i);

private:

	std::vector<Vertex> mVertices;
	std::vector<ui16> mIndicies;

};
