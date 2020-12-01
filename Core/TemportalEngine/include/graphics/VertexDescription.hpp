#pragma once

#include "TemportalEnginePCH.hpp"

#include <unordered_map>

NS_GRAPHICS

/**
 * Example:
 * struct Vertex
 * {
 *   glm::vec2 position;
 *   glm::vec3 color;
 * };
 * VertexDescription desc = {
 *   sizeof(Vertex),
 *   {
 *     { "position", CREATE_ATTRIBUTE(Vertex, position) },
 *     { "color", { offsetof(Vertex, color), sizeof(Vertex::color) } }
 *   }
 * };
 */
struct VertexDescription
{
	struct AttributeProperty
	{
		uSize offset;
		uSize size;
	};

	// Total size of 1 vertex structure (all the attributes for 1 vertex)
	ui32 size;

	// map of attribute name to the byte-offset of the property in the user's structure
	std::unordered_map<std::string, AttributeProperty> attributes;
};

#define CREATE_ATTRIBUTE(ClassType, Property) { (uSize)offsetof(ClassType, Property), (uSize)sizeof(ClassType::Property) }

NS_END
