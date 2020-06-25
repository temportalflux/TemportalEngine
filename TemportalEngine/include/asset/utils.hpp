#pragma once

#include "TemportalEnginePCH.hpp"

#include "asset/AssetType.hpp"

FORWARD_DEF(NS_ASSET, class Asset);

NS_ASSET

std::shared_ptr<asset::Asset> readAssetFromDisk(std::filesystem::path filePath, asset::EAssetSerialization type, bool bShouldHaveBeenScanned = true);

template <typename TAsset>
std::shared_ptr<TAsset> readFromDisk(std::filesystem::path filePath, asset::EAssetSerialization type, bool bShouldHaveBeenScanned = true)
{
	return std::dynamic_pointer_cast<TAsset>(asset::readAssetFromDisk(filePath, type, bShouldHaveBeenScanned));
}

NS_END
