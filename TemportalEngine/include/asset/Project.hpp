#pragma once

#include "asset/Asset.hpp"

#include "version.h"

#include <filesystem>

NS_ASSET

#define AssetType_Project "project"

class Project : public Asset
{
	friend class cereal::access;
	
public:
	DEFINE_ASSET_TYPE(AssetType_Project);

	Project() = default;
	Project(std::string name, Version version);

	std::string getName() const;
	Version getVersion() const;
	std::string getDisplayName() const;

	std::filesystem::path getAbsoluteDirectoryPath() const;
	std::filesystem::path getAssetDirectory() const;

private:
	std::string mName;
	Version mVersion;

	std::filesystem::path mProjectDirectory;

#pragma region Serialization
public:
	static std::shared_ptr<Asset> createAsset(std::filesystem::path filePath);
	static std::shared_ptr<Asset> readFromDisk(std::ifstream *stream, std::filesystem::path filePath);
	void writeToDisk(std::ofstream *stream);

private:
	template<class Archive>
	void save(Archive &archive) const
	{
		Asset::save(archive);
		archive(
			cereal::make_nvp("name", this->mName),
			cereal::make_nvp("version", this->mVersion)
		);
	}

	template<class Archive>
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
