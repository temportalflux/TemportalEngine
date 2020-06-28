#include "Model.hpp"

#include <vulkan/vulkan.hpp>

std::vector<graphics::AttributeBinding> Model::bindings(ui8 &slot)
{
	return {
		// Data per vertex of object instance
		graphics::AttributeBinding(graphics::AttributeBinding::Rate::eVertex)
		.setStructType<Vertex>()
		.addAttribute({ /*slot*/ slot++, /*vec3*/ (ui32)vk::Format::eR32G32B32Sfloat, offsetof(Vertex, position) })
		.addAttribute({ /*slot*/ slot++, /*vec3*/ (ui32)vk::Format::eR32G32B32Sfloat, offsetof(Vertex, color) })
		.addAttribute({ /*slot*/ slot++, /*vec2*/ (ui32)vk::Format::eR32G32Sfloat, offsetof(Vertex, texCoord) })
	};
}

ui16 Model::pushVertex(Vertex v)
{
	auto i = (ui16)this->mVertices.size();
	this->mVertices.push_back(v);
	return i;
}

ui16 Model::pushVertex(math::Vector3 pos, math::Vector2 texCoord)
{
	return this->pushVertex({ {pos.x(), pos.y(), pos.z()}, {0, 0, 0}, {texCoord.x(), texCoord.y()} });
}

void Model::pushIndex(ui16 i)
{
	this->mIndicies.push_back(i);
}

uSize Model::getVertexBufferSize() const
{
	return (uSize)this->mVertices.size() * sizeof(Vertex);
}

uSize Model::getIndexBufferSize() const
{
	return (uSize)this->mIndicies.size() * sizeof(ui16);
}

std::vector<Model::Vertex> Model::verticies() const
{
	return this->mVertices;
}

std::vector<ui16> Model::indicies() const
{
	return this->mIndicies;
}
