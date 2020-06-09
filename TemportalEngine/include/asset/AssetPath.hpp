#pragma once

#include "TemportalEnginePCH.hpp"

#include "asset/AssetType.hpp"

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
	AssetPath(AssetType type, std::filesystem::path path, bool bIsAbsolute = false);

	std::string toString() const;
	std::string toShortName() const;
	std::filesystem::path toAbsolutePath() const;

	template <typename Archive>
	void save(Archive &archive) const
	{
		archive(
			cereal::make_nvp("type", this->mType),
			cereal::make_nvp("path", this->mPath.string()),
			cereal::make_nvp("pathIsAbsolute", this->mbIsAbsolute)
		);
	}

	template <typename Archive>
	void load(Archive &archive)
	{
		std::string pathStr;
		archive(
			cereal::make_nvp("type", this->mType),
			cereal::make_nvp("path", pathStr),
			cereal::make_nvp("pathIsAbsolute", this->mbIsAbsolute)
		);
		this->mPath = pathStr;
	}

};

NS_END
