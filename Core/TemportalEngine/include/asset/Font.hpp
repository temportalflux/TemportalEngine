#pragma once

#include "asset/Asset.hpp"

#include "graphics/FontGlyph.hpp"

NS_ASSET

class Font : public Asset
{
public:
	DEFINE_ASSET_STATICS("font", "Font", DEFAULT_ASSET_EXTENSION, ASSET_CATEGORY_GRAPHICS);
	DECLARE_FACTORY_ASSET_METADATA()

	Font() = default;
	CREATE_NEWASSET_CONSTRUCTOR(Font) {}

	// Returns the absolute path of the font file relative to this asset path
	std::filesystem::path getFontPath() const;

public:
	struct Glyph
	{
		char asciiId;

		/**
		 * The position of the glyph from the top-left corner of the image.
		 * Dimensions are ratios of the atlas size.
		 */
		math::Vector2 atlasPos;
		/**
		 * The size of the glyph in the atlas.
		 * Dimensions are ratios of the atlas size.
		 */
		math::Vector2 atlasSize;

		f32 pointBasis; // TODO: This is per font, so it shouldn't be stored on each glyph

		math::Vector2 size;
		/**
		 * The offset/bearing of the glyph from the cursor position when drawing the glyph.
		 */
		math::Vector2 bearing;
		/**
		 * The amount to move the cursor after drawing the glyph.
		 */
		f32 advance;

		template <typename Archive>
		void save(Archive &archive) const
		{
			archive(asciiId);
			archive(atlasPos);
			archive(atlasSize);
			archive(pointBasis);
			archive(size);
			archive(bearing);
			archive(advance);
		}

		template <typename Archive>
		void load(Archive &archive)
		{
			archive(asciiId);
			archive(atlasPos);
			archive(atlasSize);
			archive(pointBasis);
			archive(size);
			archive(bearing);
			archive(advance);
		}

	};

	void setSDF(
		math::Vector2UInt const& atlasSize,
		std::vector<ui8> const& atlasPixels,
		std::vector<Glyph> const& glyphs
	);

	void getSDF(math::Vector2UInt &outAtlasSize, std::vector<ui8> &outAtlasPixels, std::vector<Glyph> &outGlyphs);

#pragma region Properties
private:

	#pragma region Json
	/**
	 * The path to the `.fnt` file (which also links to the SDF png)
	 */
	std::filesystem::path mFontPath;
	#pragma endregion

	#pragma region Binary
	math::Vector2UInt mSDFAtlasSize;
	std::vector<ui8> mSDFAtlasBinary;
	std::vector<Glyph> mSDFGlyphs;
	#pragma endregion

#pragma endregion

#pragma region Serialization
protected:
	void write(cereal::JSONOutputArchive &archive, bool bCheckDefaults) const override;
	void read(cereal::JSONInputArchive &archive, bool bCheckDefaults) override;
	void compile(cereal::PortableBinaryOutputArchive &archive, bool bCheckDefaults) const override;
	void decompile(cereal::PortableBinaryInputArchive &archive, bool bCheckDefaults) override;
#pragma endregion

};

NS_END
