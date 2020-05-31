#pragma once

#include "TemportalEnginePCH.hpp"

#include "asset/Asset.hpp"

#include <filesystem>
#include <fstream>
#include <functional>
#include <set>
#include <string>
#include <unordered_map>

NS_ASSET

struct AssetTypeMetadata
{
	std::string DisplayName;
	std::function<std::shared_ptr<Asset>(std::filesystem::path filePath)> createAsset;
	std::function<std::shared_ptr<Asset>(std::ifstream *stream, std::filesystem::path filePath)> readFromDisk;
	std::string fileExtension;
};

class AssetManager
{

public:
	static AssetManager* get();

	std::set<AssetType> getAssetTypes() const;
	AssetTypeMetadata getAssetTypeMetadata(AssetType type) const;
	std::string getAssetTypeDisplayName(AssetType type) const;
	void registerType(AssetType type, AssetTypeMetadata metadata);

	std::shared_ptr<Asset> createAsset(AssetType type, std::filesystem::path filePath);
	std::shared_ptr<Asset> readAssetFromDisk(std::filesystem::path filePath);

	// TODO: Delete these
	static void createProject(std::string filePath, std::string name) {}
	static void openProject(std::string filePath) {}
	
	void queryAssetTypes();

private:
	std::set<AssetType> mAssetTypes;
	std::unordered_map<AssetType, AssetTypeMetadata> mAssetTypeMap;

};

NS_END
