#pragma once
// TODO: Move to the graphics folder

#include "TemportalEnginePCH.hpp"

NS_GRAPHICS

// TODO: Optimize for usage
// Each AttributeDescription corresponds to a line `layout (location = <slot>) [in/out] <type> <propertyName>;`.
// Where type is used to determine `byteCount` and `format`.
struct AttributeDescription
{
	ui32 slot;
	std::string propertyName;
	std::string typeName; // only used in editor
	uSize byteCount; // not needed in release builds (used for VertexDescription verification)
	ui32 format;
};

struct ShaderMetadata
{
	std::vector<graphics::AttributeDescription> inputAttributes;
};

NS_END
