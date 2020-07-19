#pragma once

#include "TemportalEnginePCH.hpp"

#include <cereal/access.hpp>

NS_GRAPHICS

struct Area
{
	friend class cereal::access;

	math::Vector2 offset;
	math::Vector2 size;

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
