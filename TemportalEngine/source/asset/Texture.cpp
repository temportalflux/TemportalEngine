#include "asset/Texture.hpp"

#include "asset/AssetManager.hpp"
#include "cereal/list.hpp"

using namespace asset;

DEFINE_NEWASSET_FACTORY(Texture)
DEFINE_EMPTYASSET_FACTORY(Texture)

Texture::Texture(std::filesystem::path filePath) : Asset(filePath)
{
}

void Texture::setSourcePath(std::filesystem::path sourceFilePath)
{
	this->mSourceFilePath = sourceFilePath.string();
}

std::filesystem::path Texture::getAbsoluteSourcePath() const
{
	return std::filesystem::absolute(this->getPath().parent_path() / this->mSourceFilePath);
}

void Texture::setSourceBinary(std::vector<ui8> const &binary, math::Vector2UInt size)
{
	this->mSourceBinary = binary;
	this->mSourceSize = size;
}

std::vector<ui8> Texture::getSourceBinary() const
{
	return this->mSourceBinary;
}

math::Vector2UInt Texture::getSourceSize() const
{
	return this->mSourceSize;
}

#pragma region Serialization

void Texture::write(cereal::JSONOutputArchive &archive) const
{
	Asset::write(archive);
	archive(cereal::make_nvp("source", this->mSourceFilePath));
}

void Texture::read(cereal::JSONInputArchive &archive)
{
	Asset::read(archive);
	archive(cereal::make_nvp("source", this->mSourceFilePath));
}

void Texture::compile(cereal::PortableBinaryOutputArchive &archive) const
{
	Asset::compile(archive);
	// TODO: Make cerealizer for vectors
	archive(this->mSourceSize.x());
	archive(this->mSourceSize.y());
	archive(this->mSourceBinary);
}

void Texture::decompile(cereal::PortableBinaryInputArchive &archive)
{
	Asset::decompile(archive);
	archive(this->mSourceSize.x());
	archive(this->mSourceSize.y());
	archive(this->mSourceBinary);
}

#pragma endregion
