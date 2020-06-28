#pragma once

#include "asset/Asset.hpp"

#include "graphics/FontGlyph.hpp"

NS_ASSET

class Font : public Asset
{
public:
	DEFINE_ASSET_STATICS("font", "Font", DEFAULT_ASSET_EXTENSION);
	DECLARE_FACTORY_ASSET_METADATA()

	Font() = default;
	CREATE_NEWASSET_CONSTRUCTOR(Font) {}

	// Returns the absolute path of the font file relative to this asset path
	std::filesystem::path getFontPath() const;
	std::vector<ui8> getFontSizes() const;
	bool supportsFontSize(ui8 size) const;
	std::vector<graphics::FontGlyphSet>& glyphSets();
	
#pragma region Properties
private:
	#pragma region Common
	std::vector<ui8> mSupportedFontSizes;
	#pragma endregion

	#pragma region Json
	std::filesystem::path mFontPath;
	#pragma endregion

	#pragma region Binary
	// A list of glyph sets, which correspond to `mSupportedFontSizes`
	std::vector<graphics::FontGlyphSet> mGlyphSets;
	#pragma endregion

#pragma endregion

#pragma region Serialization
protected:
	void write(cereal::JSONOutputArchive &archive) const override;
	void read(cereal::JSONInputArchive &archive) override;
	void compile(cereal::PortableBinaryOutputArchive &archive) const override;
	void decompile(cereal::PortableBinaryInputArchive &archive) override;
#pragma endregion

};

NS_END
