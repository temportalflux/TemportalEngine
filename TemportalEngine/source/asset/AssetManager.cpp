#include "asset/AssetManager.hpp"

#include "Engine.hpp"
#include "logging/Logger.hpp"

#include "asset/Asset.hpp"
#include "asset/Project.hpp"

#include <filesystem>
#include <cereal/types/polymorphic.hpp>
#include <cereal/archives/json.hpp>

using namespace asset;

static logging::Logger LOG = DeclareLog("AssetManager");

AssetManager* AssetManager::get()
{
	return engine::Engine::Get()->getAssetManager();
}

void AssetManager::queryAssetTypes()
{
	this->registerType(AssetType_Project, { "Project", &Project::createAsset, &Project::readFromDisk });
}

std::set<AssetType> AssetManager::getAssetTypes() const
{
	return this->mAssetTypes;
}

std::string AssetManager::getAssetTypeDisplayName(AssetType type) const
{
	auto iter = this->mAssetTypeMap.find(type);
	assert(iter != this->mAssetTypeMap.end());
	return iter->second.DisplayName;
}

void AssetManager::registerType(AssetType type, AssetTypeMetadata metadata)
{
	this->mAssetTypes.insert(type);
	this->mAssetTypeMap.insert(std::make_pair(type, metadata));
}

std::shared_ptr<Asset> AssetManager::createAsset(AssetType type, std::filesystem::path filePath)
{
	auto typeMapEntry = this->mAssetTypeMap.find(type);
	assert(typeMapEntry != this->mAssetTypeMap.end());
	auto functor = typeMapEntry->second.createAsset;
	return functor(filePath);
}

std::shared_ptr<Asset> AssetManager::readAssetFromDisk(std::filesystem::path filePath)
{
	std::ifstream is(filePath);

	// Read the asset using the base asset class to determine the asset type that is stored
	AssetType assetType = Asset::readAsset(&is)->getAssetType();

	// Reset the file stream back to the beginning for re-deserialization
	is.clear();
	is.seekg(0, is.beg); // reset to beginning of stream

	// Determine the functor for loading this asset type
	auto typeMapEntry = this->mAssetTypeMap.find(assetType);
	assert(typeMapEntry != this->mAssetTypeMap.end());
	auto loadAssetFunctor = typeMapEntry->second.readFromDisk;

	// Actually load the asset type, using the load functor so the type is known
	return loadAssetFunctor(&is, filePath);
}
