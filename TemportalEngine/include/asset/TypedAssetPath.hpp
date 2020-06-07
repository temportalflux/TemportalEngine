#pragma once

#include "asset/AssetPath.hpp"
#include "asset/AssetManager.hpp"

NS_ASSET

template <class TAsset>
class TypedAssetPath
{

public:
	std::shared_ptr<TAsset> load() const
	{
		return AssetManager::get()->readFromDisk<TAsset>(this->mPath.toAbsolutePath());
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
