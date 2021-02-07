#pragma once

#include "asset/Asset.hpp"

NS_ASSET

/**
 * Metadata information about a generic type of asset.
 */
struct TypeData
{
	std::string DisplayName;
	std::string fileExtension;
	std::function<asset::AssetPtrStrong(std::filesystem::path filePath)> createNewAsset;
	std::function<asset::AssetPtrStrong()> createEmptyAsset;
	std::function<void(std::filesystem::path filePath)> onAssetDeleted;
};

class TypeRegistry
{

public:

	template <typename TAsset>
	void registerType()
	{
		this->registerType(TAsset::StaticType(), ASSET_TYPE_METADATA(TAsset));
	}
	void registerType(AssetType type, TypeData metadata);

	std::set<AssetType> getAssetTypes() const;
	bool isValidAssetExtension(std::string extension) const;
	TypeData getTypeData(AssetType type) const;
	std::string getTypeDisplayName(AssetType type) const;

private:
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
	std::unordered_map<AssetType, TypeData> mAssetTypeMap;


};

NS_END
