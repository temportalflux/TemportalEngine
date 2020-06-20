#include "asset/Image.hpp"

#include "asset/AssetManager.hpp"
#include "cereal/list.hpp"

using namespace asset;

DEFINE_NEWASSET_FACTORY(Image)
DEFINE_EMPTYASSET_FACTORY(Image)

Image::Image(std::filesystem::path filePath) : Asset(filePath)
{
}

void Image::setSourcePath(std::filesystem::path sourceFilePath)
{
	this->mSourceFilePath = sourceFilePath.string();
}

std::filesystem::path Image::getAbsoluteSourcePath() const
{
	return std::filesystem::absolute(this->getPath().parent_path() / this->mSourceFilePath);
}

void Image::setSourceBinary(std::vector<ui8> const &binary, math::Vector2UInt size)
{
	this->mSourceBinary = binary;
	this->mSourceSize = size;
}

#pragma region Serialization

void Image::write(cereal::JSONOutputArchive &archive) const
{
	Asset::write(archive);
	archive(cereal::make_nvp("source", this->mSourceFilePath));
}

void Image::read(cereal::JSONInputArchive &archive)
{
	Asset::read(archive);
	archive(cereal::make_nvp("source", this->mSourceFilePath));
}

void Image::compile(cereal::PortableBinaryOutputArchive &archive) const
{
	Asset::compile(archive);
	// TODO: Make cerealizer for vectors
	archive(this->mSourceSize.x());
	archive(this->mSourceSize.y());
	archive(this->mSourceBinary);
}

void Image::decompile(cereal::PortableBinaryInputArchive &archive)
{
	Asset::decompile(archive);
	archive(this->mSourceSize.x());
	archive(this->mSourceSize.y());
	archive(this->mSourceBinary);
}

#pragma endregion
