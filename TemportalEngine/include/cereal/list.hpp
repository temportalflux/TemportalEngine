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
		value.resize(size);
		for (uSize i = 0; i < size; ++i) archive(value[i]);
	}

	template <typename T>
	void save(cereal::PortableBinaryOutputArchive &archive, std::vector<T> const &value)
	{
		cereal::size_type size = value.size();
		archive(cereal::make_size_tag(size));
		archive(cereal::binary_data(value.data(), size * sizeof(T)));
	}

	template <typename T>
	void load(cereal::PortableBinaryInputArchive &archive, std::vector<T> &value)
	{
		cereal::size_type size;
		archive(cereal::make_size_tag(size));
		value.resize(size);
		archive(cereal::binary_data(value.data(), size * sizeof(T)));
	}

}
