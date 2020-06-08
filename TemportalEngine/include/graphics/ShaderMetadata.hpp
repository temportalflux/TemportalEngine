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

#ifdef WITH_EDITOR
	std::string typeName; // only used in editor
#endif

	// not needed in release builds (used for VertexDescription verification)
#ifndef NDEBUG
	uSize byteCount;
	ui32 format;
#endif

	template <typename Archive>
	void save(Archive &archive) const
	{
		archive(this->slot, this->propertyName);
#ifndef NDEBUG
		archive(this->byteCount, this->format);
#endif
	}

	template <typename Archive>
	void load(Archive &archive)
	{
		archive(this->slot, this->propertyName);
#ifndef NDEBUG
		archive(this->byteCount, this->format);
#endif
	}
};

struct ShaderMetadata
{
	std::vector<graphics::AttributeDescription> inputAttributes;
};

NS_END
