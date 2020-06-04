#include "asset/Shader.hpp"

#include "asset/AssetManager.hpp"

#include <bitset>

using namespace asset;

std::filesystem::path Shader::getSourcePathFrom(std::filesystem::path assetPath)
{
	return assetPath.parent_path() / ("." + assetPath.stem().string() + ".glsl");
}

std::shared_ptr<Asset> Shader::createAsset(std::filesystem::path filePath)
{
	auto ptr = asset::AssetManager::makeAsset<Shader>();
	ptr->mFilePath = filePath;
	ptr->writeToDisk(filePath, EAssetSerialization::Json);
	{
		std::ofstream os(Shader::getSourcePathFrom(filePath));
		os << "#version 450\n";
	}
	return ptr;
}

void Shader::onAssetDeleted(std::filesystem::path filePath)
{
	std::filesystem::remove(Shader::getSourcePathFrom(filePath));
}

std::shared_ptr<Asset> Shader::readFromDisk(std::filesystem::path filePath, EAssetSerialization type)
{
	auto ptr = asset::AssetManager::makeAsset<Shader>();
	ptr->mFilePath = filePath;
	switch (type)
	{
	case EAssetSerialization::Json:
	{
		std::ifstream is(filePath);
		cereal::JSONInputArchive archive(is);
		ptr->read(archive);
		break;
	}
	case EAssetSerialization::Binary:
	{
		std::ifstream is(filePath, std::ios::binary);
		cereal::PortableBinaryInputArchive archive(is);
		ptr->decompile(archive);
		break;
	}
	}
	return ptr;
}

void Shader::writeToDisk(std::filesystem::path filePath, EAssetSerialization type) const
{
	switch (type)
	{
	case EAssetSerialization::Json:
	{
		std::ofstream os(filePath);
		cereal::JSONOutputArchive archive(os);
		this->write(archive);
		return;
	}
	case EAssetSerialization::Binary:
	{
		std::ofstream os(filePath, std::ios::trunc | std::ios::binary);
		cereal::PortableBinaryOutputArchive archive(os);
		this->compile(archive);
		return;
	}
	}
}

void Shader::write(cereal::JSONOutputArchive &archive) const
{
	Asset::save(archive);
	archive(
		cereal::make_nvp("stage", std::bitset<32>(this->mStage).to_string())
	);
}

void Shader::read(cereal::JSONInputArchive &archive)
{
	Asset::load(archive);
	std::string stageStr;
	archive(
		cereal::make_nvp("stage", stageStr)
	);
	this->mStage = (ui32)std::bitset<32>("").to_ulong();
}

void Shader::compile(cereal::PortableBinaryOutputArchive &archive) const
{
	Asset::save(archive);
	archive(this->mStage);
}

void Shader::decompile(cereal::PortableBinaryInputArchive &archive)
{
	Asset::load(archive);
	archive(this->mStage);
}

std::string Shader::readSource() const
{
	std::ifstream is(Shader::getSourcePathFrom(this->getPath()));
	return std::string(
		std::istreambuf_iterator<char>(is),
		std::istreambuf_iterator<char>()
	);
}

void Shader::writeSource(std::string content) const
{
	std::ofstream os(Shader::getSourcePathFrom(this->getPath()), std::ios::trunc);
	os << content;
}
