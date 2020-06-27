#pragma once

#include "TemportalEnginePCH.hpp"

#include <cereal/archives/json.hpp>
#include <cereal/archives/portable_binary.hpp>
#include <cereal/cereal.hpp>

namespace cereal
{

	template <typename T>
	void save(cereal::JSONOutputArchive &archive, std::vector<T> const &value)
	{
		cereal::size_type size = value.size();
		archive(cereal::make_size_tag(size));
		for (auto const &item : value) archive(item);
	}

	template <typename T>
	void load(cereal::JSONInputArchive &archive, std::vector<T> &value)
	{
		cereal::size_type size;
		archive(cereal::make_size_tag(size));
		value.resize((uSize)size);
		for (uSize i = 0; i < size; ++i) archive(value[i]);
	}

	template <typename T>
	void save(cereal::PortableBinaryOutputArchive &archive, std::vector<T> const &value)
	{
		cereal::size_type size = value.size();
		archive(cereal::make_size_tag(size));
		// TODO: Use binary_data for primitives, and direct archive calls for complex/structures
		//archive(cereal::binary_data(value.data(), size * sizeof(T)));
		for (uSize i = 0; i < size; ++i) archive(value[i]);
	}

	template <typename T>
	void load(cereal::PortableBinaryInputArchive &archive, std::vector<T> &value)
	{
		cereal::size_type size;
		archive(cereal::make_size_tag(size));
		value.resize((uSize)size);
		//archive(cereal::binary_data(value.data(), size * sizeof(T)));
		for (uSize i = 0; i < size; ++i) archive(value[i]);
	}

	template <typename T, ui32 C>
	void save(cereal::JSONOutputArchive &archive, std::array<T, C> const &value)
	{
		cereal::size_type size = C;
		archive(cereal::make_size_tag(size));
		for (ui32 i = 0; i < C; ++i) archive(value[i]);
	}

	template <typename T, ui32 C>
	void load(cereal::JSONInputArchive &archive, std::array<T, C> &value)
	{
		cereal::size_type size;
		archive(cereal::make_size_tag(size));
		for (ui32 i = 0; i < C; ++i) archive(value[i]);
	}

	template <typename T, ui32 C>
	void save(cereal::PortableBinaryOutputArchive &archive, std::array<T, C> const &value)
	{
		cereal::size_type size = C;
		archive(cereal::make_size_tag(size));
		for (ui32 i = 0; i < C; ++i) archive(value[i]);
	}

	template <typename T, ui32 C>
	void load(cereal::PortableBinaryInputArchive &archive, std::array<T, C> &value)
	{
		cereal::size_type size;
		archive(cereal::make_size_tag(size));
		for (ui32 i = 0; i < C; ++i) archive(value[i]);
	}

}
