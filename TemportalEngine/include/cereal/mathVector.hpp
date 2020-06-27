#pragma once

#include "TemportalEnginePCH.hpp"

#include <cereal/archives/json.hpp>
#include <cereal/archives/portable_binary.hpp>
#include <cereal/cereal.hpp>
#include "math/Vector.hpp"

namespace cereal
{

	template <typename TValue, const ui8 TDimension>
	void save(cereal::JSONOutputArchive &archive, math::Vector<TValue, TDimension> const &value)
	{
		cereal::size_type size = TDimension;
		archive(cereal::make_size_tag(size));
		for (ui32 i = 0; i < size; ++i) archive(value[i]);
	}

	template <typename TValue, const ui8 TDimension>
	void load(cereal::JSONInputArchive &archive, math::Vector<TValue, TDimension> &value)
	{
		cereal::size_type size;
		archive(cereal::make_size_tag(size));
		for (ui32 i = 0; i < TDimension; ++i) archive(value[i]);
	}

	template <typename TValue, const ui8 TDimension>
	void save(cereal::PortableBinaryOutputArchive &archive, math::Vector<TValue, TDimension> const &value)
	{
		cereal::size_type size = TDimension;
		archive(cereal::make_size_tag(size));
		for (ui32 i = 0; i < size; ++i) archive(value[i]);
	}

	template <typename TValue, const ui8 TDimension>
	void load(cereal::PortableBinaryInputArchive &archive, math::Vector<TValue, TDimension> &value)
	{
		cereal::size_type size;
		archive(cereal::make_size_tag(size));
		for (ui32 i = 0; i < TDimension; ++i) archive(value[i]);
	}

}
