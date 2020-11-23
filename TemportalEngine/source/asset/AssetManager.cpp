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

template <typename TKey, typename TValue>
void multimap_erase_if(std::unordered_multimap<TKey, TValue> &map, std::function<bool(TKey const& key, TValue const& value)> predicate)
{
	for (auto it = map.begin(); it != map.end(); )
	{
		if (predicate(it->first, it->second)) it = map.erase(it);
		else ++it;
	}
}

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
	this->mActiveDirectory = directory.parent_path();

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

void AssetManager::removeScannedAsset(AssetPath metadata, std::filesystem::path absolutePath)
{
	multimap_erase_if<AssetType, AssetPath>(
		this->mScannedAssetPathsByType,
		[metadata](auto const& key, auto const& value) { return key == metadata.type() && value == metadata; }
	);
	multimap_erase_if<std::string, AssetPath>(
		this->mScannedAssetPathsByExtension,
		[metadata](auto const& key, auto const& value) { return key == metadata.extension() && value == metadata; }
	);
	this->mScannedAssetMetadataByPath.erase(this->mScannedAssetMetadataByPath.find(absolutePath.string()));
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
		multimap_erase_if<AssetPath, AssetPath>(
			this->mAssetPaths_ReferencedToReferencer,
			[referencedAssetPath, assetPath](auto const& key, auto const& value) { return key == referencedAssetPath && value == *assetPath; }
		);
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

void AssetManager::renameAsset(std::filesystem::path const& absolutePath, std::string newName)
{
	auto assetPath = this->getAssetMetadata(absolutePath);
	assert(assetPath);
	auto newAssetPath = AssetPath(assetPath->type(), assetPath->path().parent_path() / (newName + assetPath->path().extension().string()));
	this->moveAsset(*assetPath, newAssetPath);
}

void AssetManager::moveAsset(AssetPath path, std::filesystem::path const& newParent)
{
	auto oldAbsolutePath = this->mActiveDirectory / path.path();
	auto newAbsolutePath = newParent / path.path().filename();
	auto newAssetPath = AssetPath(path.type(), std::filesystem::relative(newAbsolutePath, this->mActiveDirectory));
	this->moveAsset(path, newAssetPath);
}

void AssetManager::moveAsset(AssetPath const& prev, AssetPath const& next)
{
	auto oldAbsolutePath = this->mActiveDirectory / prev.path();
	auto newAbsolutePath = this->mActiveDirectory / next.path();
	// Update any assets which reference the asset being moved
	while (true)
	{
		auto iter = this->mAssetPaths_ReferencedToReferencer.find(prev);
		if (iter == this->mAssetPaths_ReferencedToReferencer.end()) break;
		auto referencerPath = std::filesystem::absolute(this->mActiveDirectory / iter->second.path());
		auto referencerAsset = asset::readAssetFromDisk(referencerPath, EAssetSerialization::Json);
		referencerAsset->replaceAssetReference(prev, next);
		referencerAsset->writeToDisk(referencerPath, EAssetSerialization::Json);
	}

	// Remove any references from the asset being moved to other assets
	this->mAssetPaths_ReferencerToReferenced.erase(prev);

	// Load the asset so extra data can be moved and asset references can be updated
	std::unordered_set<AssetPath> referenceAssets;
	{
		auto asset = this->getAssetTypeMetadata(prev.type()).createEmptyAsset();
		asset->readFromDisk(oldAbsolutePath, EAssetSerialization::Json);
		// move any extra data for things like shaders and textures, which store the actual content in a different file
		asset->onPreMoveAsset(oldAbsolutePath, newAbsolutePath);
		// save any data that changed during `onPreMoveAsset` (like import paths)
		asset->writeToDisk(oldAbsolutePath, EAssetSerialization::Json);
		referenceAssets = asset->getReferencedAssetPaths();
	}

	// Remove scanned asset (so the asset path is no longer in the manager)
	this->removeScannedAsset(prev, oldAbsolutePath);

	// Actually move the asset
	std::filesystem::rename(oldAbsolutePath, newAbsolutePath);

	// Re-add the asset and asset references
	this->addScannedAsset(next, newAbsolutePath);
	this->setAssetReferences(newAbsolutePath, referenceAssets);
}
