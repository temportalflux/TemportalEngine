#include "asset/AssetManager.hpp"

#include "Engine.hpp"
#include "logging/Logger.hpp"

#include "asset/Asset.hpp"
#include "asset/Project.hpp"
#include "asset/Shader.hpp"
#include "asset/Texture.hpp"

#include <filesystem>
#include <cereal/types/polymorphic.hpp>
#include <cereal/archives/json.hpp>

using namespace asset;

static logging::Logger LOG = DeclareLog("AssetManager");

std::shared_ptr<AssetManager> AssetManager::get()
{
	return engine::Engine::Get()->getAssetManager();
}

void AssetManager::setAssetMemory(std::shared_ptr<memory::MemoryChunk> assetMemory)
{
	this->mpAssetMemory = assetMemory;
}

std::shared_ptr<memory::MemoryChunk> AssetManager::getAssetMemory() const
{
	return this->mpAssetMemory;
}

void AssetManager::queryAssetTypes()
{
	this->registerType(AssetType_Project, CREATE_ASSETTYPE_METADATA(Project, "Project", ".te-project", std::nullopt));
	this->registerType(AssetType_Shader, CREATE_ASSETTYPE_METADATA(Shader, "Shader", ".te-asset", &Shader::onAssetDeleted));
	this->registerType(AssetType_Image, CREATE_ASSETTYPE_METADATA(Texture, "Texture", ".te-asset", std::nullopt));
}

void AssetManager::scanAssetDirectory(std::filesystem::path directory, asset::EAssetSerialization type)
{
	this->mScannedAssetPathsByExtension.clear();
	this->mScannedAssetMetadataByPath.clear();

	ui32 totalFilesScanned = 0, foundAssetCount = 0;
	for (const auto& entry : std::filesystem::recursive_directory_iterator(directory))
	{
		// Ignore any non-normal files (directories, symlinks, etc.)
		if (!entry.is_regular_file()) continue;

		totalFilesScanned++;

		// Ignore all files whose extension is not known
		if (!this->isValidAssetExtension(entry.path().extension().string())) continue;

		foundAssetCount++;

		AssetType assetType;
		{
			auto asset = makeAsset<Asset>();
			asset->readFromDisk(entry.path(), type);
			assetType = asset->getAssetType();
		}

		this->addScannedAsset(AssetPath(assetType, std::filesystem::relative(entry.path(), directory.parent_path())), entry.path());
	}

	LOG.log(logging::ECategory::LOGINFO, "Scanned %i files and found %i assets", totalFilesScanned, foundAssetCount);
}

void AssetManager::addScannedAsset(AssetPath metadata, std::filesystem::path absolutePath)
{
	this->mScannedAssetPathsByExtension.insert(std::make_pair(metadata.extension(), metadata));
	this->mScannedAssetMetadataByPath.insert(std::make_pair(absolutePath.string(), metadata));
}

std::set<AssetType> AssetManager::getAssetTypes() const
{
	return this->mAssetTypes;
}

bool AssetManager::isValidAssetExtension(std::string extension) const
{
	return this->mAssetTypeExtensions.find(extension) != this->mAssetTypeExtensions.end();
}

AssetTypeMetadata AssetManager::getAssetTypeMetadata(AssetType type) const
{
	auto iter = this->mAssetTypeMap.find(type);
	assert(iter != this->mAssetTypeMap.end());
	return iter->second;
}

std::string AssetManager::getAssetTypeDisplayName(AssetType type) const
{
	return this->getAssetTypeMetadata(type).DisplayName;
}

void AssetManager::registerType(AssetType type, AssetTypeMetadata metadata)
{
	// Assumes that the type has not been registered before
	assert(this->mAssetTypes.find(type) == this->mAssetTypes.end());
	// Add type and metadata to mappings
	this->mAssetTypes.insert(type);
	this->mAssetTypeMap.insert(std::make_pair(type, metadata));
	// Ensure that if the extension isn't already cataloged, it gets added to the set
	if (!this->isValidAssetExtension(metadata.fileExtension))
	{
		this->mAssetTypeExtensions.insert(metadata.fileExtension);
	}
}

std::optional<AssetPath> AssetManager::getAssetMetadata(std::filesystem::path filePath) const
{
	auto iter = this->mScannedAssetMetadataByPath.find(filePath.string());
	return iter == this->mScannedAssetMetadataByPath.end() ? std::nullopt : std::make_optional(iter->second);
}

std::vector<AssetPath> AssetManager::getAssetList() const
{
	auto paths = std::vector<AssetPath>();
	std::transform(
		this->mScannedAssetPathsByExtension.begin(),
		this->mScannedAssetPathsByExtension.end(),
		std::back_inserter(paths), [](auto pair) { return pair.second; }
	);
	return paths;
}

std::vector<AssetPath> AssetManager::getAssetList(AssetType type) const
{
	auto metadata = this->getAssetTypeMetadata(type);
	auto iters = this->mScannedAssetPathsByExtension.equal_range(metadata.fileExtension);
	auto paths = std::vector<AssetPath>(std::distance(iters.first, iters.second));
	std::transform(iters.first, iters.second, paths.begin(), [](auto pair) { return pair.second; });
	return paths;
}

std::shared_ptr<Asset> AssetManager::createAsset(AssetType type, std::filesystem::path filePath)
{
	auto typeMapEntry = this->mAssetTypeMap.find(type);
	assert(typeMapEntry != this->mAssetTypeMap.end());
	auto asset = typeMapEntry->second.createNewAsset(filePath);
	asset->writeToDisk(filePath, EAssetSerialization::Json);
	this->addScannedAsset({ type, filePath }, filePath);
	return asset;
}

void AssetManager::deleteFile(std::filesystem::path filePath)
{
	if (this->isValidAssetExtension(filePath.extension().string()))
	{
		auto assetMeta = this->getAssetMetadata(filePath);
		if (!assetMeta.has_value())
		{
			LOG.log(logging::ECategory::LOGWARN, "Deleting asset %s which has not been scanned.", filePath.string().c_str());
		}
		else
		{
			auto assetTypeMeta = this->getAssetTypeMetadata(assetMeta.value().type());
			if (assetTypeMeta.onAssetDeleted.has_value())
			{
				assetTypeMeta.onAssetDeleted.value()(filePath);
			}
		}
	}
	std::filesystem::remove(filePath);
}
