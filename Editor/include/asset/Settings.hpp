#pragma once

#include "asset/Asset.hpp"

NS_ASSET

#define AssetType_EditorSettings "editorSettings"

class Settings : public Asset
{
	friend class cereal::access;

public:
	DEFINE_ASSET_TYPE(AssetType_EditorSettings);

	Settings() = default;
	Settings(std::filesystem::path filePath);

	// Returns the relative path from `asset::Project::getAbsoluteDirectoryPath`
	std::string getOutputDirectory() const;
	void setOutputDirectory(std::string path);

private:
	// A relative path from `asset::Project::getAbsoluteDirectoryPath` to which asset files should be built
	std::string mOutputDirectoryPath;

#pragma region Serialization
public:
	static std::shared_ptr<Asset> createAsset(std::filesystem::path filePath);
	static std::shared_ptr<Asset> readFromDisk(std::filesystem::path filePath, EAssetSerialization type);
	void writeToDisk(std::filesystem::path filePath, EAssetSerialization type) const override;

private:

	template <typename Archive>
	void save(Archive &archive) const
	{
		Asset::save(archive);
		archive(
			cereal::make_nvp("outputDirectory", this->mOutputDirectoryPath)
		);
	}

	template <typename Archive>
	void load(Archive &archive)
	{
		Asset::load(archive);
		archive(
			cereal::make_nvp("outputDirectory", this->mOutputDirectoryPath)
		);
	}
#pragma endregion
};

NS_END
