#pragma once

#include "TemportalEnginePCH.hpp"

#include "asset/AssetType.hpp"
#include <cereal/cereal.hpp>

NS_ASSET

/**
 * The identifier of an asset based on their registered `AssetType` and a path relative to the project's root directory.
 */
class AssetPath
{

private:
	AssetType mType;
	std::filesystem::path mPath;
	bool mbIsAbsolute;

public:
	AssetPath() : mbIsAbsolute(false) {}
	AssetPath(AssetType type, std::filesystem::path path, bool bIsAbsolute = false);

	operator bool() const { return this->isValid(); }
	bool isValid() const;
	AssetType type() const { return this->mType; }
	std::string pathStr() const { return this->mPath.string(); }
	std::string toString() const;
	std::string toShortName() const;
	std::string filename() const;
	std::string extension() const;
	std::filesystem::path toAbsolutePath() const;
	AssetPath& loadFromString(std::string fullStr)
	{
		auto delimiter = fullStr.find(':');
		if (delimiter != std::string::npos)
		{
			this->mType = fullStr.substr(0, delimiter);
			this->mPath = fullStr.substr(delimiter + 1);
		}
		return *this;
	}

	bool operator==(AssetPath const &other) const
	{
		return this->toAbsolutePath() == other.toAbsolutePath();
	}

	static AssetPath fromString(std::string fullStr)
	{
		return AssetPath().loadFromString(fullStr);
	}

	template <typename Archive>
	std::string save_minimal(Archive const& archive) const
	{
		return this->toString();
	}

	template <typename Archive>
	void load_minimal(Archive const& archive, std::string const& value)
	{
		this->loadFromString(value);
	}

};

NS_END

namespace std
{
	template<>
	struct hash<asset::AssetPath>
	{
		inline size_t operator()(asset::AssetPath const &id) const
		{
			return std::hash<std::string>()(id.toString());
		}
	};
}