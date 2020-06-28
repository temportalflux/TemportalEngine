#pragma once

#include "TemportalEnginePCH.hpp"

#include "graphics/AttributeBinding.hpp"
#include "math/Vector.hpp"

#include "glm/glm.hpp"

class Model
{

public:
	struct Vertex
	{
		glm::vec3 position;
		glm::vec3 color;
		glm::vec2 texCoord;
	};

	static std::vector<graphics::AttributeBinding> bindings(ui8 &slot);

	uSize getVertexBufferSize() const;
	uSize getIndexBufferSize() const;
	std::vector<Vertex> verticies() const;
	std::vector<ui16> indicies() const;

protected:
	ui16 pushVertex(Vertex v);
	ui16 pushVertex(math::Vector3 pos, math::Vector2 texCoord);
	void pushIndex(ui16 i);

private:

	std::vector<Vertex> mVertices;
	std::vector<ui16> mIndicies;

};
