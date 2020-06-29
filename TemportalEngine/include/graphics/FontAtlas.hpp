#pragma once

#include "TemportalEnginePCH.hpp"

#include "graphics/FontGlyph.hpp"

NS_GRAPHICS

class Font
{
	struct GlyphSprite : public FontGlyphMeta
	{
		math::Vector2UInt atlasOffset;

		GlyphSprite& operator=(FontGlyph const &other);
	};

public:
	class Face
	{
		friend class Font;
	public:
		math::Vector2UInt measure(std::string str) const;
	private:
		ui8 fontSize;
		std::unordered_map<ui32, ui32> codeToGlyphIdx;
		std::vector<GlyphSprite> glyphs;
		math::Vector2UInt atlasSize;
		void loadGlyphSet(FontGlyphSet const &src);
		// Determines the glyph offsets and atlas size
		math::Vector2UInt calculateAtlasLayout();
	};

	void loadGlyphSets(std::vector<ui8> const &fontSizes, std::vector<graphics::FontGlyphSet> const &glyphSets);
	Face& getFace(ui8 size);

private:
	std::vector<ui8> mSupportedSizes;
	std::vector<Face> mGlyphFaces;

	std::optional<uIndex> findSet(ui8 size) const;

};

NS_END
