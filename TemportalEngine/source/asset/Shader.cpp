#include "asset/Shader.hpp"

#include "Engine.hpp"
#include "asset/AssetManager.hpp"
#include "graphics/ShaderModule.hpp"
#include "memory/MemoryChunk.hpp"
#include "cereal/list.hpp"
#include "cereal/GraphicsFlags.hpp"

#include <bitset>
#include <vulkan/vulkan.hpp>

using namespace asset;

DEFINE_FACTORY_ASSET_NEW(Shader)
DEFINE_FACTORY_ASSET_EMPTY(Shader)

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

ui32 Shader::getStage() const
{
	return this->mStage;
}

void Shader::setStage(ui32 value)
{
	this->mStage = value;
}

std::filesystem::path Shader::getSourcePathFrom(std::filesystem::path assetPath)
{
	return assetPath.parent_path() / ("." + assetPath.stem().string() + ".glsl");
}

#pragma endregion

#pragma region Source Maintinence

void Shader::setBinary(std::vector<ui32> binary, graphics::ShaderMetadata metadata)
{
	this->mSourceBinary = binary;
	this->mBinaryMetadata = metadata;
}

std::optional<graphics::ShaderMetadata> Shader::getMetadata()
{
	return !this->mSourceBinary.empty() ? std::make_optional(this->mBinaryMetadata) : std::nullopt;
}

void Shader::writeSource(std::string content) const
{
	std::ofstream os(Shader::getSourcePathFrom(this->getPath()), std::ios::out | std::ios::trunc);
	os << content;
}

std::string Shader::readSource() const
{
	std::ifstream is(Shader::getSourcePathFrom(this->getPath()), std::ios::in);
	auto str = std::string(
		std::istreambuf_iterator<char>(is),
		std::istreambuf_iterator<char>()
	);
	return str;
}

#pragma endregion

#pragma region Serialization

void Shader::write(cereal::JSONOutputArchive &archive) const
{
	Asset::write(archive);
	archive(cereal::make_nvp("stage", (vk::ShaderStageFlagBits const&)this->mStage));
}

void Shader::read(cereal::JSONInputArchive &archive)
{
	Asset::read(archive);
	archive(cereal::make_nvp("stage", (vk::ShaderStageFlagBits&)this->mStage));
}

void Shader::compile(cereal::PortableBinaryOutputArchive &archive) const
{
	Asset::compile(archive);
	archive((vk::ShaderStageFlagBits const&)this->mStage);
	archive(this->mSourceBinary);
	archive(this->mBinaryMetadata.inputAttributes);
}

void Shader::decompile(cereal::PortableBinaryInputArchive &archive)
{
	Asset::decompile(archive);
	archive((vk::ShaderStageFlagBits&)this->mStage);
	archive(this->mSourceBinary);
	archive(this->mBinaryMetadata.inputAttributes);
}

#pragma endregion

// TODO: Needs a ui8 (0/1) for vk::VertexInputRate
std::shared_ptr<graphics::ShaderModule> Shader::makeModule()
{
	// TODO: Change this to dedicated graphics memory
	auto mem = engine::Engine::Get()->getMiscMemory();
	auto ptr = mem->make_shared<graphics::ShaderModule>();
	ptr->setStage((vk::ShaderStageFlagBits)this->mStage);
	ptr->setBinary(this->mSourceBinary);
	ptr->setMetadata(this->mBinaryMetadata);
	return ptr;
}
