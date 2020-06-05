#pragma once

#include "TemportalEnginePCH.hpp"

NS_GRAPHICS

// Each AttributeDescription corresponds to a line `layout (location = <slot>) [in/out] <type> <propertyName>;`.
// Where type is used to determine `byteCount` and `format`.
struct AttributeDescription
{
	ui32 slot;
	std::string propertyName;
	uSize byteCount;
	ui32 format;
};

struct ShaderMetadata
{
	std::vector<graphics::AttributeDescription> inputAttributes;
};

NS_END
