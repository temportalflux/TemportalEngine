#pragma once

#include "TemportalEnginePCH.hpp"

#include <cereal/access.hpp>

NS_GRAPHICS

struct Area
{
	friend class cereal::access;

	math::Vector2 offset;
	math::Vector2 size;

	Area() = default;
	Area(math::Vector2 offset, math::Vector2 size) : offset(offset), size(size) {}

	bool operator==(Area const& other) const { return offset == other.offset && size == other.size; }
	bool operator!=(Area const& other) const { return !(*this == other); }

	template <typename Archive>
	void save(Archive &archive) const
	{
		archive(cereal::make_nvp("offset", this->offset));
		archive(cereal::make_nvp("size", this->size));
	}

	template <typename Archive>
	void load(Archive &archive)
	{
		archive(cereal::make_nvp("offset", this->offset));
		archive(cereal::make_nvp("size", this->size));
	}
};

struct Viewport : public Area
{
	friend class cereal::access;

	math::Vector2 depthRange;

	Viewport() = default;
	Viewport(math::Vector2 offset, math::Vector2 size, math::Vector2 depthRange) : Area(offset, size), depthRange(depthRange) {}

	bool operator==(Viewport const& other) const { return offset == other.offset && size == other.size && depthRange == other.depthRange; }
	bool operator!=(Viewport const& other) const { return !(*this == other); }

	template <typename Archive>
	void save(Archive &archive) const
	{
		Area::save(archive);
		archive(cereal::make_nvp("depthRange", this->depthRange));
	}

	template <typename Archive>
	void load(Archive &archive)
	{
		Area::load(archive);
		archive(cereal::make_nvp("depthRange", this->depthRange));
	}

};

NS_END
