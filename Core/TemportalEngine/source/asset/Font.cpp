#include "asset/Font.hpp"

#include "asset/AssetManager.hpp"
#include "memory/MemoryChunk.hpp"
#include "cereal/list.hpp"
#include "cereal/mathVector.hpp"

using namespace asset;

DEFINE_FACTORY_ASSET_METADATA(Font)

std::filesystem::path Font::getFontPath() const
{
	return std::filesystem::absolute(this->getPath().parent_path() / this->mFontPath);
}

void Font::setSDF(
	math::Vector2UInt const& atlasSize,
	std::vector<ui8> const& atlasPixels,
	std::vector<Font::Glyph> const& glyphs
)
{
	this->mSDFAtlasSize = atlasSize;
	this->mSDFAtlasBinary = atlasPixels;
	this->mSDFGlyphs = glyphs;
}

void Font::getSDF(
	math::Vector2UInt &outAtlasSize,
	std::vector<ui8> &outAtlasPixels,
	std::vector<Glyph> &outGlyphs
)
{
	outAtlasSize = this->mSDFAtlasSize;
	outAtlasPixels = this->mSDFAtlasBinary;
	outGlyphs = this->mSDFGlyphs;
}

#pragma region Serialization

void Font::write(cereal::JSONOutputArchive &archive, bool bCheckDefaults) const
{
	Asset::write(archive, bCheckDefaults);
	archive(cereal::make_nvp("path", this->mFontPath.string()));
}

void Font::read(cereal::JSONInputArchive &archive, bool bCheckDefaults)
{
	Asset::read(archive, bCheckDefaults);
	std::string pathStr;
	archive(cereal::make_nvp("path", pathStr));
	this->mFontPath = pathStr;
}

void Font::compile(cereal::PortableBinaryOutputArchive &archive, bool bCheckDefaults) const
{
	Asset::compile(archive, bCheckDefaults);
	archive(this->mSDFAtlasSize);
	archive(this->mSDFAtlasBinary);
	archive(this->mSDFGlyphs);
}

void Font::decompile(cereal::PortableBinaryInputArchive &archive, bool bCheckDefaults)
{
	Asset::decompile(archive, bCheckDefaults);
	archive(this->mSDFAtlasSize);
	archive(this->mSDFAtlasBinary);
	archive(this->mSDFGlyphs);
}

#pragma endregion
