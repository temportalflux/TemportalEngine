#pragma once

#include "TemportalEnginePCH.hpp"

#include "asset/AssetType.hpp"
#include "asset/TypedAssetPath.hpp"
#include "asset/AssetHelper.hpp"

#include <cereal/access.hpp>
#include <cereal/archives/json.hpp>
#include <cereal/archives/portable_binary.hpp>
#include <cereal/cereal.hpp>
#include <cereal/types/common.hpp>
#include <cereal/types/string.hpp>
#include <filesystem>
#include <fstream>
#include <string>
#include <cassert>

NS_ASSET

class Asset
{
	friend class cereal::access;

public:
	Asset() = default;
	// createAsset constructor
	Asset(std::filesystem::path filePath);

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
	void writeToDisk(std::filesystem::path filePath, EAssetSerialization type) const;
	void readFromDisk(std::filesystem::path filePath, EAssetSerialization type);
protected:
	virtual DECLARE_SERIALIZATION_METHOD(write, cereal::JSONOutputArchive, const);
	virtual DECLARE_SERIALIZATION_METHOD(read, cereal::JSONInputArchive, );
	virtual DECLARE_SERIALIZATION_METHOD(compile, cereal::PortableBinaryOutputArchive, const);
	virtual DECLARE_SERIALIZATION_METHOD(decompile, cereal::PortableBinaryInputArchive, );

	template <typename Archive>
	void serialize(Archive &archive) const
	{
		archive(cereal::make_nvp("type", this->getAssetType()));
	}

	template <typename Archive>
	void deserialize(Archive &archive)
	{
		archive(cereal::make_nvp("type", this->mAssetType));
	}
#pragma endregion

};

typedef std::shared_ptr<Asset> AssetPtrStrong;

NS_END
