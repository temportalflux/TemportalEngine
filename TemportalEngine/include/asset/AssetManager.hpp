#pragma once

#include "TemportalEnginePCH.hpp"

#include "asset/Asset.hpp"
#include "asset/TypedAssetPath.hpp"
#include "memory/MemoryChunk.hpp"

#include <filesystem>
#include <fstream>
#include <functional>
#include <optional>
#include <set>
#include <string>
#include <unordered_map>
#include <map>

NS_ASSET

/**
 * Metadata information about a generic type of asset.
 */
struct AssetTypeMetadata
{
	std::string DisplayName;
	std::string fileExtension;
	std::function<asset::AssetPtrStrong(std::filesystem::path filePath)> createNewAsset;
	std::function<asset::AssetPtrStrong()> createEmptyAsset;
	std::function<void(std::filesystem::path filePath)> onAssetDeleted;
};

class AssetManager
{

public:
	typedef std::unordered_multimap<AssetPath, AssetPath> ReferenceMap;
	typedef std::pair<ReferenceMap::const_iterator, ReferenceMap::const_iterator> ReferenceIterRange;
	static std::shared_ptr<AssetManager> get();

	std::shared_ptr<memory::MemoryChunk> mpAssetMemory;
	void setAssetMemory(std::shared_ptr<memory::MemoryChunk> assetMemory);
	std::shared_ptr<memory::MemoryChunk> getAssetMemory() const;
	
	template <typename T, typename... Types>
	static std::shared_ptr<T> makeAsset(Types&& ...args)
	{
		return AssetManager::get()->mpAssetMemory->make_shared<T>(args...);
	}

	/**
	 * Registers all known asset types.
	 * Should only ever be called once.
	 */
	void queryAssetTypes();

	/**
	 * Recursively scans all files in the directory and catalogs any files
	 * whose extensions are known (based on asset types cataloged by `registerType`).
	 * Performs a read operation on each file with a known extension to determine its `AssetType`.
	 * Generally collects `AssetPath`.
	 */
	void scanAssetDirectory(std::filesystem::path directory, asset::EAssetSerialization type);

	template <typename TAsset>
	void registerType()
	{
		this->registerType(TAsset::StaticType(), ASSET_TYPE_METADATA(TAsset));
	}
	void registerType(AssetType type, AssetTypeMetadata metadata);
	std::set<AssetType> getAssetTypes() const;
	bool isValidAssetExtension(std::string extension) const;

	AssetTypeMetadata getAssetTypeMetadata(AssetType type) const;
	std::string getAssetTypeDisplayName(AssetType type) const;
	std::optional<AssetPath> getAssetMetadata(std::filesystem::path filePath) const;
	AssetPath* getAssetMetadataPtr(std::filesystem::path filePath);

	std::vector<AssetPath> getAssetList() const;
	std::vector<AssetPath> getAssetList(AssetType type) const;
	template <typename TAsset>
	std::vector<TypedAssetPath<TAsset>> getAssetList() const
	{
		auto pathList = getAssetList(TAsset::StaticType());
		auto typedList = std::vector<TypedAssetPath<TAsset>>(pathList.size());
		std::transform(pathList.begin(), pathList.end(), typedList.begin(), [](auto path) { return TypedAssetPath<TAsset>(path); });
		return typedList;
	}

	std::shared_ptr<Asset> createAsset(AssetType type, std::filesystem::path filePath);
	void deleteFile(std::filesystem::path filePath);

	template <typename TAsset>
	std::shared_ptr<TAsset> createAssetAs(AssetType type, std::filesystem::path filePath)
	{
		return std::dynamic_pointer_cast<TAsset>(this->createAsset(type, filePath));
	}

	void setAssetReferences(std::filesystem::path absolutePath, std::unordered_set<AssetPath> const& paths);
	ReferenceIterRange getAssetPathsReferencedBy(std::filesystem::path const& absolutePath) const;
	ReferenceIterRange getAssetPathsWhichReference(std::filesystem::path const& absolutePath) const;
	bool isAssetReferenced(std::filesystem::path const& absolutePath) const;

private:
	std::filesystem::path mActiveDirectory;

	/**
	 * A unique set of all known asset types.
	 * New types can be registered with `registerType`.
	 */
	std::set<AssetType> mAssetTypes;
	/**
	 * A list of all asset extensions.
	 * Populated when an asset type is registered via `registerType`.
	 */
	std::set<std::string> mAssetTypeExtensions;
	/**
	 * A mapping of a unique asset type to its metadata information.
	 * Populated when an asset type is registered via `registerType`.
	 */
	std::unordered_map<AssetType, AssetTypeMetadata> mAssetTypeMap;

	/**
	 * A map of asset extension (see `AssetTypeMetadata::fileExtension`) to all known asset paths (see `mScannedAssetMetadataByPath`).
	 * Initially populated by `scanAssetDirectory`.
	 * Can be modified when assets are created by `createAsset` or when they are destroyed (TODO: Assets cannot yet be destroyed without editing files directly).
	 */
	std::unordered_multimap<std::string, AssetPath> mScannedAssetPathsByExtension;
	/**
	 * A map of an asset's path (relative to the directory passed to `scanAssetDirectory`) to all known assets.
	 * Initially populated by `scanAssetDirectory`.
	 * Can be modified when assets are created by `createAsset` or when they are destroyed (TODO: Assets cannot yet be destroyed without editing files directly).
	 */
	// TODO: This is so redundant. Its a map of path to path.
	std::unordered_map<std::string, AssetPath> mScannedAssetMetadataByPath;
	std::unordered_multimap<AssetType, AssetPath> mScannedAssetPathsByType;
	ReferenceMap mAssetPaths_ReferencerToReferenced;
	ReferenceMap mAssetPaths_ReferencedToReferencer;

	void addScannedAsset(AssetPath metadata, std::filesystem::path absolutePath);

};

NS_END
