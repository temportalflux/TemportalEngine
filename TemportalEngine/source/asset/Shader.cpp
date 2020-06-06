#include "asset/Shader.hpp"

#include "Engine.hpp"
#include "asset/AssetManager.hpp"
#include "graphics/ShaderModule.hpp"
#include "memory/MemoryChunk.hpp"

#include <bitset>
#include <vulkan/vulkan.hpp>

using namespace asset;

DEFINE_NEWASSET_FACTORY(Shader)
DEFINE_EMPTYASSET_FACTORY(Shader)

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
	
	uSize len;

	// Source binary
	len = (uSize)this->mSourceBinary.size();
	archive(len);
	archive(cereal::binary_data(this->mSourceBinary.data(), len * sizeof(ui32)));

	// Binary Metadata
	len = (uSize)this->mBinaryMetadata.inputAttributes.size();
	archive(len);
	for (auto& attrib : this->mBinaryMetadata.inputAttributes)
	{
		archive(attrib.slot, attrib.propertyName, attrib.byteCount, attrib.format);
	}
	
}

void Shader::decompile(cereal::PortableBinaryInputArchive &archive)
{
	Asset::decompile(archive);
	archive(this->mStage);

	uSize len;
	
	// Source Binary
	archive(len);
	this->mSourceBinary.resize(len);
	archive(cereal::binary_data(this->mSourceBinary.data(), len * sizeof(ui32)));

	// Binary Metadata
	archive(len);
	this->mBinaryMetadata.inputAttributes.resize(len);
	for (uSize i = 0; i < len; ++i)
	{
		archive(this->mBinaryMetadata.inputAttributes[i].slot);
		archive(this->mBinaryMetadata.inputAttributes[i].propertyName);
		archive(this->mBinaryMetadata.inputAttributes[i].byteCount);
		archive(this->mBinaryMetadata.inputAttributes[i].format);
	}

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
