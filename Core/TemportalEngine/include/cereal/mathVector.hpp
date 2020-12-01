#pragma once

#include "TemportalEnginePCH.hpp"

#include <cereal/archives/json.hpp>
#include <cereal/archives/portable_binary.hpp>
#include <cereal/cereal.hpp>
#include "math/Vector.hpp"

namespace cereal
{

	template <typename TValue, const ui8 TAccessibleDimensions, const ui8 TActualDimensions>
	void save(cereal::JSONOutputArchive &archive, math::Vector<TValue, TAccessibleDimensions, TActualDimensions> const &value)
	{
		cereal::size_type size = TAccessibleDimensions;
		archive(cereal::make_size_tag(size));
		for (ui32 i = 0; i < size; ++i) archive(value[i]);
	}

	template <typename TValue, const ui8 TAccessibleDimensions, const ui8 TActualDimensions>
	void load(cereal::JSONInputArchive &archive, math::Vector<TValue, TAccessibleDimensions, TActualDimensions> &value)
	{
		cereal::size_type size;
		archive(cereal::make_size_tag(size));
		for (ui32 i = 0; i < TAccessibleDimensions; ++i) archive(value[i]);
	}

	template <typename TValue, const ui8 TAccessibleDimensions, const ui8 TActualDimensions>
	void save(cereal::PortableBinaryOutputArchive &archive, math::Vector<TValue, TAccessibleDimensions, TActualDimensions> const &value)
	{
		cereal::size_type size = TAccessibleDimensions;
		archive(cereal::make_size_tag(size));
		for (ui32 i = 0; i < size; ++i) archive(value[i]);
	}

	template <typename TValue, const ui8 TAccessibleDimensions, const ui8 TActualDimensions>
	void load(cereal::PortableBinaryInputArchive &archive, math::Vector<TValue, TAccessibleDimensions, TActualDimensions> &value)
	{
		cereal::size_type size;
		archive(cereal::make_size_tag(size));
		for (ui32 i = 0; i < TAccessibleDimensions; ++i) archive(value[i]);
	}

}
