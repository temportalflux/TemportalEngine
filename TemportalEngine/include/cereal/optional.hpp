#pragma once

#include "TemportalEnginePCH.hpp"

#include <cereal/archives/json.hpp>
#include <cereal/archives/portable_binary.hpp>
#include <cereal/cereal.hpp>

namespace cereal
{

	template <typename T>
	void save(cereal::JSONOutputArchive &archive, std::optional<T> const &value)
	{
		archive(cereal::make_nvp("hasValue", (bool)value));
		if (value)
		{
			archive(cereal::make_nvp("value", *value));
		}
	}

	template <typename T>
	void load(cereal::JSONInputArchive &archive, std::optional<T> &value)
	{
		bool bHasValue;
		archive(cereal::make_nvp("hasValue", bHasValue));
		if (bHasValue)
		{
			T v;
			archive(cereal::make_nvp("value", v));
			value = v;
		}
	}

	template <typename T>
	void save(cereal::PortableBinaryOutputArchive &archive, std::optional<T> const &value)
	{
		archive((bool)value);
		if (value) archive(*value);
	}

	template <typename T>
	void load(cereal::PortableBinaryInputArchive &archive, std::optional<T> &value)
	{
		bool bHasValue;
		archive(bHasValue);
		if (bHasValue)
		{
			T v;
			archive(v);
			value = v;
		}
	}

}
