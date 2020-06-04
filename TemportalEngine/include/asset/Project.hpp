#pragma once

#include "asset/Asset.hpp"

#include "version.h"

NS_ASSET

#define AssetType_Project "project"

class Project : public Asset
{
	friend class cereal::access;
	
public:
	DEFINE_ASSET_TYPE(AssetType_Project);

	static std::filesystem::path getAssetDirectoryFor(std::filesystem::path projectDir);

	Project() = default;
	Project(std::filesystem::path path);
	Project(std::string name, Version version);

	std::string getName() const;
	void setName(std::string value);
	Version getVersion() const;
	void setVersion(Version value);
	std::string getDisplayName() const;

	std::filesystem::path getAbsoluteDirectoryPath() const;
	std::filesystem::path getAssetDirectory() const;

private:
	std::string mName;
	Version mVersion;

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
			cereal::make_nvp("name", this->mName),
			cereal::make_nvp("version", this->mVersion)
		);
	}

	template <typename Archive>
	void load(Archive &archive)
	{
		Asset::load(archive);
		archive(
			cereal::make_nvp("name", this->mName),
			cereal::make_nvp("version", this->mVersion)
		);
	}
#pragma endregion

};

typedef std::shared_ptr<Project> ProjectPtrStrong;
typedef std::weak_ptr<Project> ProjectPtrWeak;

NS_END
