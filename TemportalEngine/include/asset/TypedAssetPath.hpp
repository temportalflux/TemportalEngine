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
	static AssetType StaticType() { return TAsset::StaticType(); }

	static TypedAssetPath<TAsset> Create(std::filesystem::path path)
	{
		return TypedAssetPath<TAsset>(AssetPath(StaticType(), path, path.is_absolute()));
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

	std::shared_ptr<TAsset> load(asset::EAssetSerialization type, bool bShouldHaveBeenScanned = true) const
	{
		return asset::readFromDisk<TAsset>(this->mPath.toAbsolutePath(), type, bShouldHaveBeenScanned);
	}

	template <typename Archive>
	std::string save_minimal(Archive const& archive) const
	{
		return this->toString();
	}

	template <typename Archive>
	void load_minimal(Archive const& archive, std::string const& value)
	{
		this->mPath.loadFromString(value);
	}

private:
	AssetPath mPath;

};

NS_END
