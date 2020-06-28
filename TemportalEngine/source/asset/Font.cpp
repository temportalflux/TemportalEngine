#include "asset/Font.hpp"

#include "asset/AssetManager.hpp"
#include "memory/MemoryChunk.hpp"
#include "cereal/list.hpp"

using namespace asset;

DEFINE_FACTORY_ASSET_METADATA(Font)

std::filesystem::path Font::getFontPath() const
{
	return std::filesystem::absolute(this->getPath().parent_path() / this->mFontPath);
}

std::vector<ui8> Font::getFontSizes() const
{
	return this->mSupportedFontSizes;
}

bool Font::supportsFontSize(ui8 size) const
{
	return std::find(this->mSupportedFontSizes.begin(), this->mSupportedFontSizes.end(), size) != this->mSupportedFontSizes.end();
}

std::vector<graphics::FontGlyphSet>& Font::glyphSets()
{
	return this->mGlyphSets;
}

#pragma region Serialization

void Font::write(cereal::JSONOutputArchive &archive) const
{
	Asset::write(archive);
	archive(cereal::make_nvp("path", this->mFontPath.string()));
	archive(cereal::make_nvp("fontSizes", this->mSupportedFontSizes));
}

void Font::read(cereal::JSONInputArchive &archive)
{
	Asset::read(archive);
	std::string pathStr;
	archive(cereal::make_nvp("path", pathStr));
	this->mFontPath = pathStr;
	archive(cereal::make_nvp("fontSizes", this->mSupportedFontSizes));
}

void Font::compile(cereal::PortableBinaryOutputArchive &archive) const
{
	Asset::compile(archive);
	archive(this->mSupportedFontSizes);
	archive(this->mGlyphSets);
}

void Font::decompile(cereal::PortableBinaryInputArchive &archive)
{
	Asset::decompile(archive);
	archive(this->mSupportedFontSizes);
	archive(this->mGlyphSets);
}

#pragma endregion
