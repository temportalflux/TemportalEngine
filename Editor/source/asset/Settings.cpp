#include "asset/Settings.hpp"

#include "asset/AssetManager.hpp"

using namespace asset;

std::string Settings::getOutputDirectory() const
{
	return this->mOutputDirectoryPath;
}

void Settings::setOutputDirectory(std::string path)
{
	this->mOutputDirectoryPath = path;
}

#pragma region Serialization

std::shared_ptr<Asset> Settings::createAsset(std::filesystem::path filePath)
{
	auto ptr = asset::AssetManager::makeAsset<Settings>();
	ptr->mFilePath = filePath;
	ptr->writeToDisk(filePath, EAssetSerialization::Json);
	return ptr;
}

std::shared_ptr<Asset> Settings::readFromDisk(std::filesystem::path filePath, EAssetSerialization type)
{
	auto ptr = asset::AssetManager::makeAsset<Settings>();
	switch (type)
	{
	case EAssetSerialization::Json:
	{
		std::ifstream is(filePath);
		cereal::JSONInputArchive archive(is);
		ptr->load(archive);
		break;
	}
	default: break; // not supported
	}
	ptr->mFilePath = filePath;
	return ptr;
}

void Settings::writeToDisk(std::filesystem::path filePath, EAssetSerialization type) const
{
	switch (type)
	{
	case EAssetSerialization::Json:
	{
		std::ofstream os(filePath);
		cereal::JSONOutputArchive archive(os, Asset::JsonFormat);
		this->save(archive);
		return;
	}
	default: break; // not supported
	}
}

#pragma endregion
