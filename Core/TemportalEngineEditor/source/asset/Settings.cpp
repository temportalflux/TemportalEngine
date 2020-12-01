#include "asset/Settings.hpp"

#include "asset/AssetManager.hpp"

using namespace asset;

DEFINE_FACTORY_ASSET_METADATA(Settings)

#pragma region Properties

std::string Settings::getOutputDirectory() const
{
	return this->mOutputDirectoryPath;
}

void Settings::setOutputDirectory(std::string path)
{
	this->mOutputDirectoryPath = path;
}

#pragma endregion

#pragma region Serialization

void Settings::write(cereal::JSONOutputArchive &archive, bool bCheckDefaults) const
{
	Asset::write(archive, bCheckDefaults);
	archive(
		cereal::make_nvp("outputDirectory", this->mOutputDirectoryPath)
	);
}

void Settings::read(cereal::JSONInputArchive &archive, bool bCheckDefaults)
{
	Asset::read(archive, bCheckDefaults);
	archive(
		cereal::make_nvp("outputDirectory", this->mOutputDirectoryPath)
	);
}

#pragma endregion
