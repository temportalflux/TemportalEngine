#include "asset/Asset.hpp"

#include "asset/AssetManager.hpp"
#include "memory/MemoryChunk.hpp"

#include <algorithm>
#include <cereal/archives/json.hpp>
#include <cereal/archives/portable_binary.hpp>

using namespace asset;

cereal::JSONOutputArchive::Options Asset::JsonFormat = cereal::JSONOutputArchive::Options(
	324,
	cereal::JSONOutputArchive::Options::IndentChar::tab, 1
);

Asset::Asset(std::filesystem::path filePath) : mFilePath(filePath)
{
}

std::shared_ptr<Asset> Asset::readAsset(std::filesystem::path filePath, asset::EAssetSerialization type)
{
	auto ptr = asset::AssetManager::makeAsset<Asset>();
	switch (type)
	{
	case EAssetSerialization::Json:
	{
		std::ifstream is(filePath);
		cereal::JSONInputArchive archive(is);
		ptr->load(archive);
		break;
	}
	case EAssetSerialization::Binary:
	{
		std::ifstream is(filePath, std::ios::binary);
		cereal::PortableBinaryInputArchive archive(is);
		ptr->load(archive);
		break;
	}
	}
	ptr->mFilePath = filePath;
	return ptr;
}

void Asset::writeToDisk(std::filesystem::path filePath, EAssetSerialization type) const
{
	// Should always be overriden by subclasses
	assert(false);
}

std::filesystem::path Asset::getPath() const
{
	return this->mFilePath;
}

std::string Asset::getFileName() const
{
	return this->mFilePath.stem().string();
}

/*
std::string Asset::toString()
{
	std::stringstream ss;
	{
		cereal::JSONOutputArchive archive(ss, cereal::JSONOutputArchive::Options::NoIndent());
		//std::shared_ptr<Asset> asset = std::make_shared<Asset>(this);
		//archive(cereal::make_nvp(asset->getAssetType(), asset));
	}
	auto stringified = ss.str();
	// strip out all new lines
	stringified.erase(std::remove(stringified.begin(), stringified.end(), '\n'), stringified.end());
	return stringified;
}
//*/
