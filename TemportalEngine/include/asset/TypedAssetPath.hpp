#pragma once

#include "TemportalEnginePCH.hpp"

#include "asset/AssetPath.hpp"
#include "asset/utils.hpp"
#include <cereal/cereal.hpp>

NS_ASSET

template <class TAsset>
class TypedAssetPath
{
public:
	static TypedAssetPath<TAsset> Create(std::filesystem::path path)
	{
		return TypedAssetPath<TAsset>(AssetPath(TAsset::StaticType(), path, path.is_absolute()));
	}

	TypedAssetPath() = default;
	TypedAssetPath(AssetPath path) : mPath(path) {}

	static TypedAssetPath<TAsset> fromString(std::string fullStr)
	{
		return TypedAssetPath<TAsset>(AssetPath::fromString(fullStr));
	}

	AssetType getTypeFilter() const { return TAsset::StaticType(); }

	std::string toString() const { return this->mPath.toString(); }
	AssetPath& path() { return this->mPath; }

	std::shared_ptr<TAsset> load(asset::EAssetSerialization type, bool bShouldHaveBeenScanned = true)
	{
		return asset::readFromDisk<TAsset>(this->mPath.toAbsolutePath(), type, bShouldHaveBeenScanned);
	}

	template <typename Archive>
	void write(std::string name, Archive &archive) const
	{
		archive(cereal::make_nvp(name, this->toString()));
	}

	template <typename Archive>
	void read(std::string name, Archive &archive)
	{
		std::string fullStr;
		archive(cereal::make_nvp(name, fullStr));
		this->mPath = AssetPath::fromString(fullStr);
	}

private:
	AssetPath mPath;

};

NS_END
