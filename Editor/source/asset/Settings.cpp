#include "asset/Settings.hpp"

#include "asset/AssetManager.hpp"

using namespace asset;

asset::AssetPtrStrong Settings::createNewAsset(std::filesystem::path filePath)
{
	return asset::AssetManager::makeAsset<Settings>(filePath);
}

asset::AssetPtrStrong Settings::createEmptyAsset()
{
	return asset::AssetManager::makeAsset<Settings>();
}

Settings::Settings(std::filesystem::path filePath) : Asset(filePath)
{
}

std::string Settings::getOutputDirectory() const
{
	return this->mOutputDirectoryPath;
}

void Settings::setOutputDirectory(std::string path)
{
	this->mOutputDirectoryPath = path;
}

#pragma region Serialization

void Settings::write(cereal::JSONOutputArchive &archive) const
{
	Asset::write(archive);
	archive(
		cereal::make_nvp("outputDirectory", this->mOutputDirectoryPath)
	);
}

void Settings::read(cereal::JSONInputArchive &archive)
{
	Asset::read(archive);
	archive(
		cereal::make_nvp("outputDirectory", this->mOutputDirectoryPath)
	);
}


#pragma endregion
