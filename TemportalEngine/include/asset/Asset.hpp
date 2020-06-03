#pragma once

#include "TemportalEnginePCH.hpp"

#include "asset/AssetType.hpp"

#include <cereal/access.hpp>
#include <cereal/archives/json.hpp>
#include <cereal/archives/portable_binary.hpp>
#include <cereal/cereal.hpp>
#include <cereal/types/common.hpp>
#include <cereal/types/string.hpp>
#include <filesystem>
#include <fstream>
#include <string>

NS_ASSET

#define DEFINE_ASSET_TYPE(TYPE_STR) virtual AssetType getAssetType() const override { return TYPE_STR; }

enum class EAssetSerialization
{
	Json,
	Binary,
};

class Asset
{
	friend class cereal::access;

public:
	/**
	 * The type of this asset. Should be overridden by subclasses using `DEFINE_ASSET_TYPE("my-asset-type");`.
	 * Used when the asset is written to disk to define the type loaded into `mAssetType` when the asset is loaded.
	 */
	virtual AssetType getAssetType() const { return this->mAssetType; }

	std::filesystem::path getPath() const;
	std::string getFileName() const;

protected:
	static cereal::JSONOutputArchive::Options JsonFormat;

	/**
	 * The location of this asset relative to the asset directory.
	 */
	std::filesystem::path mFilePath;

private:
	/**
	 * The known type of this asset.
	 * This may be empty if the asset has not yet be saved to disk.
	 * When an asset is loaded from disk, the type value saved via
	 * `getAssetType` will be loaded into this field.
	 */
	AssetType mAssetType;

#pragma region Serialization
public:
	/**
	 * Reads the asset data from the file stream (calling `Asset#load` in the process).
	 */
	static std::shared_ptr<Asset> readAsset(std::filesystem::path filePath, asset::EAssetSerialization type);
	virtual void writeToDisk(std::filesystem::path filePath, EAssetSerialization type) const;

protected:

	template<class Archive>
	void save(Archive &archive) const
	{
		// Always write the constant-type-per-subclass as the type
		archive(cereal::make_nvp("type", this->getAssetType()));
	}

	template<class Archive>
	void load(Archive &archive)
	{
		// Always load the type into the local field
		archive(cereal::make_nvp("type", this->mAssetType));
	}
#pragma endregion

};

typedef std::shared_ptr<Asset> AssetPtrStrong;

NS_END
