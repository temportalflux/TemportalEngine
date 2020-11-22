#include "asset/Texture.hpp"

#include "asset/AssetManager.hpp"
#include "cereal/list.hpp"

using namespace asset;

DEFINE_FACTORY_ASSET_METADATA(Texture)

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

uSize Texture::getSourceMemorySize() const
{
	return this->mSourceSize.x() * this->mSourceSize.y() * 4 * sizeof(ui8);
}

void Texture::onPreMoveAsset(std::filesystem::path const& prevAbsolute, std::filesystem::path const& nextAbsolute)
{
	auto srcImportPath = std::filesystem::path(this->mSourceFilePath);
	if (srcImportPath.parent_path() == prevAbsolute.parent_path())
	{
		std::filesystem::rename(srcImportPath, std::filesystem::absolute(nextAbsolute.parent_path() / srcImportPath.filename()));
	}
}

#pragma region Serialization

void Texture::write(cereal::JSONOutputArchive &archive, bool bCheckDefaults) const
{
	Asset::write(archive, bCheckDefaults);
	archive(cereal::make_nvp("source", this->mSourceFilePath));
}

void Texture::read(cereal::JSONInputArchive &archive, bool bCheckDefaults)
{
	Asset::read(archive, bCheckDefaults);
	archive(cereal::make_nvp("source", this->mSourceFilePath));
}

void Texture::compile(cereal::PortableBinaryOutputArchive &archive, bool bCheckDefaults) const
{
	Asset::compile(archive, bCheckDefaults);
	// TODO: Make cerealizer for vectors
	archive(this->mSourceSize.x());
	archive(this->mSourceSize.y());
	archive(this->mSourceBinary);
}

void Texture::decompile(cereal::PortableBinaryInputArchive &archive, bool bCheckDefaults)
{
	Asset::decompile(archive, bCheckDefaults);
	archive(this->mSourceSize.x());
	archive(this->mSourceSize.y());
	archive(this->mSourceBinary);
}

#pragma endregion
