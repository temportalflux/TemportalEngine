#include "asset/Shader.hpp"

#include "asset/AssetManager.hpp"

#include <bitset>
#include <shaderc/shaderc.hpp>
#include <vulkan/vulkan.hpp>

using namespace asset;

asset::AssetPtrStrong Shader::createNewAsset(std::filesystem::path filePath)
{
	return asset::AssetManager::makeAsset<Shader>(filePath);
}

asset::AssetPtrStrong Shader::createEmptyAsset()
{
	return asset::AssetManager::makeAsset<Shader>();
}

void Shader::onAssetDeleted(std::filesystem::path filePath)
{
	std::filesystem::remove(Shader::getSourcePathFrom(filePath));
}

Shader::Shader(std::filesystem::path filePath) : Asset(filePath)
{
	// Initialize the source file
	{
		std::ofstream os(Shader::getSourcePathFrom(filePath));
		os << "#version 450\n";
	}
}

#pragma region Properties

std::filesystem::path Shader::getSourcePathFrom(std::filesystem::path assetPath)
{
	return assetPath.parent_path() / ("." + assetPath.stem().string() + ".glsl");
}

#pragma endregion

#pragma region Serialization

void Shader::write(cereal::JSONOutputArchive &archive) const
{
	Asset::write(archive);
	archive(
		cereal::make_nvp("stage", std::bitset<32>(this->mStage).to_string())
	);
}

void Shader::read(cereal::JSONInputArchive &archive)
{
	Asset::read(archive);
	std::string stageStr;
	archive(
		cereal::make_nvp("stage", stageStr)
	);
	this->mStage = (ui32)std::bitset<32>(stageStr).to_ulong();
}

void Shader::compile(cereal::PortableBinaryOutputArchive &archive) const
{
	Asset::compile(archive);
	archive(this->mStage);
}

void Shader::decompile(cereal::PortableBinaryInputArchive &archive)
{
	Asset::decompile(archive);
	archive(this->mStage);
}

#pragma endregion

#pragma region Source Maintinence

std::string Shader::readSource() const
{
	std::ifstream is(Shader::getSourcePathFrom(this->getPath()), std::ios::in);
	auto str = std::string(
		std::istreambuf_iterator<char>(is),
		std::istreambuf_iterator<char>()
	);
	return str;
}

void Shader::writeSource(std::string content) const
{
	std::ofstream os(Shader::getSourcePathFrom(this->getPath()), std::ios::out | std::ios::trunc);
	os << content;
}

#pragma endregion
