#pragma once

#include "TemportalEnginePCH.hpp"

#include <cereal/archives/json.hpp>
#include <cereal/archives/portable_binary.hpp>
#include <cereal/cereal.hpp>

NS_UTILITY

template <typename TEnum>
class EnumWrapper
{
public:
	static std::vector<TEnum> ALL;

	EnumWrapper() = default;
	constexpr EnumWrapper(TEnum value) : mValue(value) {}
	constexpr EnumWrapper(ui64 value) : EnumWrapper(TEnum(value)) {}

	operator ui64() const { return ui64(mValue); }
	operator TEnum() const { return mValue; }
	TEnum& value() { return mValue; }
	TEnum const& value() const { return mValue; }
	explicit operator bool() = delete;
	constexpr bool operator<(EnumWrapper<TEnum> const& other) const { return mValue < other.mValue; }
	constexpr bool operator==(EnumWrapper<TEnum> const& other) const { return mValue == other.mValue; }
	constexpr bool operator!=(EnumWrapper<TEnum> const& other) const { return mValue != other.mValue; }

	template <typename TAs>
	TAs as() const { return TAs(value()); }

	std::string to_string() const;
	std::string to_display_string() const;

private:
	TEnum mValue;
};

template <typename TFlagType>
struct Flags
{
	typedef EnumWrapper<TFlagType> TFlagEnum;
	constexpr std::vector<TFlagType> all() const { return TFlagEnum::ALL; }

	Flags() = default;

	Flags(ui64 data) : mData(data) { updateSet(); updateString(); }

	Flags(TFlagType option)
	{
		this->mData = ui64(option);
		this->updateSet();
		this->updateString();
	}
	
	Flags(std::unordered_set<TFlagType> const& set)
	{
		this->mSet = set;
		this->mData = 0;
		for (TFlagType option : set)
		{
			this->mData |= ((ui64)option);
		}
		this->updateString();
	}

	std::unordered_set<TFlagType> const& toSet() const { return this->mSet; }

	void updateSet()
	{
		this->mSet = std::unordered_set<TFlagType>();
		for (TFlagType option : all())
		{
			if (static_cast<ui64>(option) & mData)
			{
				this->mSet.insert(option);
			}
		}
	}

	void operator|=(TFlagType const& option)
	{
		(*this) |= ui64(option);
	}

	void operator|=(ui64 const& option)
	{
		this->mData |= option;
		this->mSet.insert(TFlagType(option));
		this->updateString();
	}

	Flags<TFlagType> operator|(TFlagType const& option)
	{
		auto flags = Flags<TFlagType>(*this);
		flags |= option;
		return flags;
	}

	Flags<TFlagType> operator|(ui64 const& option)
	{
		auto flags = Flags<TFlagType>(*this);
		flags |= option;
		return flags;
	}

	void operator&=(TFlagType const& option)
	{
		(*this) &= ui64(option);
	}

	void operator&=(ui64 const& option)
	{
		this->mData &= option;
		this->updateSet();
		this->updateString();
	}

	Flags<TFlagType> operator&(TFlagType const& option)
	{
		auto flags = Flags<TFlagType>(*this);
		flags &= option;
		return flags;
	}

	Flags<TFlagType> operator&(ui64 const& option)
	{
		auto flags = Flags<TFlagType>(*this);
		flags &= option;
		return flags;
	}

	void operator^=(TFlagType const& option)
	{
		(*this) ^= ui64(option);
	}

	void operator^=(ui64 const& option)
	{
		this->mData ^= option;
		this->updateSet();
		this->updateString();
	}

	Flags<TFlagType> operator^(TFlagType const& option)
	{
		auto flags = Flags<TFlagType>(*this);
		flags ^= option;
		return flags;
	}

	Flags<TFlagType> operator^(ui64 const& option)
	{
		auto flags = Flags<TFlagType>(*this);
		flags ^= option;
		return flags;
	}

	bool operator==(Flags<TFlagType> const& other) const { return mData == other.mData; }

	ui64 const& data() const { return mData; }
	ui64& data() { return mData; }

	operator TFlagType() const { return TFlagType(data()); }
	std::string const& to_string() const { return this->mCombinedString; }

private:
	ui64 mData;
	std::unordered_set<TFlagType> mSet;
	std::string mCombinedString;

	void updateString()
	{
		std::stringstream sstream;
		for (TFlagType option : all())
		{
			if (static_cast<ui64>(option) & mData)
			{
				sstream << TFlagEnum(option).to_string();
			}
		}
		this->mCombinedString = sstream.str();
	}

};

NS_END

namespace std
{
	template <typename T>
	struct hash<utility::EnumWrapper<T>>
	{
		inline size_t operator()(utility::EnumWrapper<T> const& value) const
		{
			return size_t(value.value());
		}
	};
}
