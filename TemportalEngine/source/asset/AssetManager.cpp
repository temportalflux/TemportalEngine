#include "asset/AssetManager.hpp"

#include "Engine.hpp"
#include "logging/Logger.hpp"

#include "asset/Asset.hpp"
#include "asset/Font.hpp"
#include "asset/PipelineAsset.hpp"
#include "asset/Project.hpp"
#include "asset/RenderPassAsset.hpp"
#include "asset/Shader.hpp"
#include "asset/Texture.hpp"
#include "asset/TextureSampler.hpp"

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
	this->registerType<Font>();
	this->registerType<Pipeline>();
	this->registerType<Project>();
	this->registerType<RenderPass>();
	this->registerType<Shader>();
	this->registerType<Texture>();
	this->registerType<TextureSampler>();
}

void AssetManager::scanAssetDirectory(std::filesystem::path directory, asset::EAssetSerialization type)
{
	this->mActiveDirectory = directory;

	this->mScannedAssetPathsByType.clear();
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

		auto assetPath = AssetPath(assetType, std::filesystem::relative(entry.path(), directory.parent_path()));
		this->addScannedAsset(assetPath, entry.path());

		// Read the assets in their actual type to determine any asset references
		{
			auto asset = this->getAssetTypeMetadata(assetType).createEmptyAsset();
			asset->readFromDisk(entry.path(), type);
			this->setAssetReferences(entry.path(), asset->getReferencedAssetPaths());
		}
	}

	LOG.log(logging::ECategory::LOGINFO, "Scanned %i files and found %i assets", totalFilesScanned, foundAssetCount);
}

void AssetManager::addScannedAsset(AssetPath metadata, std::filesystem::path absolutePath)
{
	this->mScannedAssetPathsByType.insert(std::make_pair(metadata.type(), metadata));
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

AssetPath* AssetManager::getAssetMetadataPtr(std::filesystem::path filePath)
{
	auto iter = this->mScannedAssetMetadataByPath.find(filePath.string());
	return iter == this->mScannedAssetMetadataByPath.end() ? nullptr : &iter->second;
}

std::vector<AssetPath> AssetManager::getAssetList() const
{
	auto paths = std::vector<AssetPath>();
	std::transform(
		this->mScannedAssetPathsByType.begin(),
		this->mScannedAssetPathsByType.end(),
		std::back_inserter(paths), [](auto pair) { return pair.second; }
	);
	return paths;
}

std::vector<AssetPath> AssetManager::getAssetList(AssetType type) const
{
	auto iters = this->mScannedAssetPathsByType.equal_range(type);
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
			assetTypeMeta.onAssetDeleted(filePath);
		}
	}
	std::filesystem::remove(filePath);
}

void AssetManager::setAssetReferences(std::filesystem::path absolutePath, std::unordered_set<AssetPath> const& paths)
{
	auto assetPath = this->getAssetMetadata(absolutePath);
	assert(assetPath);

	auto iterReferenced = this->mAssetPaths_ReferencerToReferenced.equal_range(*assetPath);
	for (auto iter = iterReferenced.first; iter != iterReferenced.second; ++iter)
	{
		AssetPath const& referencedAssetPath = iter->second;
		for (auto it = this->mAssetPaths_ReferencedToReferencer.begin(); it != this->mAssetPaths_ReferencedToReferencer.end(); )
		{
			if (it->first == referencedAssetPath && it->second == *assetPath)
				it = this->mAssetPaths_ReferencedToReferencer.erase(it);
			else ++it;
		}
	}

	this->mAssetPaths_ReferencerToReferenced.erase(*assetPath);
	for (auto const& path : paths)
	{
		this->mAssetPaths_ReferencerToReferenced.insert(std::make_pair(*assetPath, path));
		this->mAssetPaths_ReferencedToReferencer.insert(std::make_pair(path, *assetPath));
	}
}

AssetManager::ReferenceIterRange AssetManager::getAssetPathsReferencedBy(std::filesystem::path const& absolutePath) const
{
	auto assetPath = this->getAssetMetadata(absolutePath);
	assert(assetPath);
	return this->mAssetPaths_ReferencerToReferenced.equal_range(*assetPath);
}

AssetManager::ReferenceIterRange AssetManager::getAssetPathsWhichReference(std::filesystem::path const& absolutePath) const
{
	auto assetPath = this->getAssetMetadata(absolutePath);
	assert(assetPath);
	return this->mAssetPaths_ReferencedToReferencer.equal_range(*assetPath);
}

bool AssetManager::isAssetReferenced(std::filesystem::path const& absolutePath) const
{
	auto iterRange = this->getAssetPathsWhichReference(absolutePath);
	return iterRange.first != this->mAssetPaths_ReferencedToReferencer.end();
}
