#pragma once

#include "TemportalEnginePCH.hpp"

#include <cereal/archives/json.hpp>
#include <cereal/archives/portable_binary.hpp>
#include <cereal/cereal.hpp>

NS_UTILITY

template <typename TFlagType>
struct Flags
{

	Flags() = default;
	
	Flags(std::unordered_set<TFlagType> const& set)
	{
		this->mData = 0;
		for (TFlagType option : set)
		{
			this->mData |= ((ui64)option);
		}
	}

	std::unordered_set<TFlagType> toSet(std::vector<TFlagType> const& all) const
	{
		auto flags = std::unordered_set<TFlagType>();
		for (TFlagType option : all)
		{
			if (static_cast<ui64>(option) & mData)
			{
				flags.insert(option);
			}
		}
		return flags;
	}

	void operator|=(TFlagType const& option)
	{
		this->mData |= ((ui64)option);
	}

	Flags<TFlagType> operator|(TFlagType const& option)
	{
		auto flags = Flags<TFlagType>(*this);
		flags |= option;
		return flags;
	}

	void operator&=(TFlagType const& option)
	{
		this->mData &= ((ui64)option);
	}

	Flags<TFlagType> operator&(TFlagType const& option)
	{
		auto flags = Flags<TFlagType>(*this);
		flags &= option;
		return flags;
	}

	ui64 const& data() const { return mData; }
	ui64& data() { return mData; }

private:
	ui64 mData;

};

NS_END
