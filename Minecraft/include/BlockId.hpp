#pragma once

#include "CoreInclude.hpp"

NS_GAME

struct BlockId
{

	BlockId() = default;
	BlockId(std::string moduleName, std::string name) : moduleName(moduleName), name(name) {}

	// TODO: Replace with an asset referenced
	std::string moduleName;

	/**
	 * The unique name of the block type.
	 */
	std::string name;

	bool operator==(BlockId const &other) const
	{
		return this->to_string() == other.to_string();
	}

	std::string const to_string() const
	{
		return this->moduleName + ":" + this->name;
	}

	template <typename Archive>
	void save(Archive &archive) const
	{
		archive(cereal::make_nvp("module", this->moduleName));
		archive(cereal::make_nvp("name", this->name));
	}

	template <typename Archive>
	void load(Archive &archive)
	{
		archive(cereal::make_nvp("module", this->moduleName));
		archive(cereal::make_nvp("name", this->name));
	}
};

NS_END

namespace std
{
	template<>
	struct hash<game::BlockId>
	{
		inline size_t operator()(game::BlockId const &id) const
		{
			return std::hash<std::string>()(id.to_string());
		}
	};
}
