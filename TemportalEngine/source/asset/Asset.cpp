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

#pragma region Properties

AssetPath Asset::assetPath() const
{
	return AssetPath(this->mAssetType, this->mFilePath, true);
}

std::filesystem::path Asset::getPath() const
{
	return this->mFilePath;
}

std::string Asset::getFileName() const
{
	return this->mFilePath.stem().string();
}

#pragma endregion

#pragma region Serialization

void Asset::writeToDisk(std::filesystem::path filePath, EAssetSerialization type) const
{
	std::filesystem::create_directories(filePath.parent_path());
	switch (type)
	{
	case EAssetSerialization::Json:
	{
		std::ofstream os(filePath);
		cereal::JSONOutputArchive archive(os, Asset::JsonFormat);
		this->write(archive, true);
		return;
	}
	case EAssetSerialization::Binary:
	{
		std::ofstream os(filePath, std::ios::binary);
		cereal::PortableBinaryOutputArchive archive(os);
		this->compile(archive, false);
		return;
	}
	}
}

void Asset::readFromDisk(std::filesystem::path filePath, asset::EAssetSerialization type)
{
	this->mFilePath = filePath;
	switch (type)
	{
	case EAssetSerialization::Json:
	{
		std::ifstream is(filePath);
		cereal::JSONInputArchive archive(is);
		this->read(archive, true);
		break;
	}
	case EAssetSerialization::Binary:
	{
		std::ifstream is(filePath, std::ios::binary);
		cereal::PortableBinaryInputArchive archive(is);
		this->decompile(archive, false);
		break;
	}
	}
}

CREATE_DEFAULT_SERIALIZATION_DEFINITION(const, Asset::write, cereal::JSONOutputArchive, Asset::serialize);
CREATE_DEFAULT_SERIALIZATION_DEFINITION(, Asset::read, cereal::JSONInputArchive, Asset::deserialize);
CREATE_DEFAULT_SERIALIZATION_DEFINITION(const, Asset::compile, cereal::PortableBinaryOutputArchive, Asset::serialize);
CREATE_DEFAULT_SERIALIZATION_DEFINITION(, Asset::decompile, cereal::PortableBinaryInputArchive, Asset::deserialize);

#pragma endregion

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
