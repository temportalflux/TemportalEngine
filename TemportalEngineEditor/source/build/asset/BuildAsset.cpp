#include "build/asset/BuildAsset.hpp"

#include "Editor.hpp"
#include "asset/Asset.hpp"

using namespace build;

std::shared_ptr<BuildAsset> BuildAsset::create(std::shared_ptr<asset::Asset> asset)
{
	return std::make_shared<BuildAsset>(asset);
}

BuildAsset::BuildAsset(std::shared_ptr<asset::Asset> asset) : mpAsset(asset)
{
}

std::shared_ptr<asset::Asset> BuildAsset::getAsset() const
{
	return this->mpAsset;
}

void BuildAsset::setOutputPath(std::filesystem::path const &path)
{
	this->mOutputPath = path;
}

std::vector<std::string> BuildAsset::compile(logging::Logger &logger)
{
	// No-op for generic assets
	return {};
}

void BuildAsset::save()
{
	this->mpAsset->writeToDisk(this->mOutputPath, asset::EAssetSerialization::Binary);
}
