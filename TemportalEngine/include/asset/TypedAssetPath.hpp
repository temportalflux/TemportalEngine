#pragma once

#include "TemportalEnginePCH.hpp"

#include "asset/AssetPath.hpp"
#include "asset/AssetManager.hpp"

NS_ASSET

template <class TAsset>
class TypedAssetPath
{

public:
	TypedAssetPath(AssetPath path) : mPath(path) {}

	std::shared_ptr<TAsset> load(EAssetSerialization serialization, bool bWasScanned=true) const
	{
		return AssetManager::get()->readFromDisk<TAsset>(this->mPath.toAbsolutePath(), serialization, bWasScanned);
	}

	template <typename Archive>
	void save(Archive &archive) const
	{
		this->mPath.save(archive);
	}

	template <typename Archive>
	void load(Archive &archive)
	{
		this->mPath.load(archive);
	}

private:
	AssetPath mPath;

};

NS_END
