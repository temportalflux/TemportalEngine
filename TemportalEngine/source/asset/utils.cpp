#include "asset/utils.hpp"

#include "asset/Asset.hpp"
#include "asset/AssetManager.hpp"
#include "logging/Logger.hpp"
#include "Engine.hpp"

std::shared_ptr<asset::Asset> asset::readAssetFromDisk(std::filesystem::path filePath, asset::EAssetSerialization type, bool bShouldHaveBeenScanned)
{
	filePath = filePath.make_preferred();
	assert(std::filesystem::exists(filePath));
	
	auto assetManager = asset::AssetManager::get();
	if (!assetManager->isValidAssetExtension(filePath.extension().string()))
	{
		return nullptr;
	}

	// Read the asset using the base asset class to determine the asset type that is stored
	auto assetMetadata = assetManager->getAssetMetadata(filePath);
	AssetType assetType = assetMetadata ? assetMetadata->type : "invalid";
	if (!assetMetadata)
	{
		if (bShouldHaveBeenScanned)
			DeclareLog("AssetManager").log(logging::ECategory::LOGWARN, "Reading asset %s which has not been scanned.", filePath.string().c_str());
		auto asset = asset::AssetManager::makeAsset<Asset>();
		asset->readFromDisk(filePath, type);
		assetType = asset->getAssetType();
	}

	// Determine the functor for loading this asset type
	auto asset = assetManager->getAssetTypeMetadata(assetType).createEmptyAsset();
	asset->readFromDisk(filePath, type);
	return asset;
}
