#pragma once

#include "TemportalEnginePCH.hpp"

#include "graphics/FontGlyph.hpp"
#include "graphics/Image.hpp"
#include "graphics/ImageSampler.hpp"
#include "graphics/ImageView.hpp"

NS_GRAPHICS

class Font
{
	struct GlyphSprite : public FontGlyphMeta
	{
		math::Vector2UInt atlasOffset;

		GlyphSprite& operator=(FontGlyph const &other);
	};

public:
	struct UIVertex
	{
		// <x,y> is the position of the string
		// <z,w> is the offset from the string position of this vertex, composed from `FontGlyphMeta#metricsOffset` and `FontGlyphMeta#advance`
		// These two vec2s will be summed in the shader when rendering the vertex
		math::Vector4 position;
		math::Vector2 texCoord;
	};
	struct GlyphData
	{
		math::Vector2 bearing;
		math::Vector2 size;
		f32 advance;
		math::Vector2 uvPos;
		math::Vector2 uvSize;
	};

	class Face
	{
		friend class Font;
	public:
		ui8 getFontSize() const { return this->fontSize; }
		std::pair<math::Vector2UInt, math::Vector2Int> measure(std::string const& str) const;
		math::Vector2UInt getAtlasSize() const;
		std::vector<ui8>& getPixelData();
		std::optional<GlyphData> getGlyph(char const& charCode) const;
	private:
		ui8 fontSize;
		std::unordered_map<ui32, ui32> codeToGlyphIdx;
		std::vector<GlyphSprite> glyphs;
		math::Vector2UInt atlasSize;
		std::vector<ui8> textureData;
		void loadGlyphSet(FontGlyphSet const &src);
		// Determines the glyph offsets and atlas size
		math::Vector2UInt calculateAtlasLayout();
		void writeAlphaToTexture(math::Vector2UInt const &pos, math::Vector2UInt const &dimensions, std::vector<ui8> const &alpha);
	};

	Font& loadGlyphSets(std::vector<ui8> const &fontSizes, std::vector<graphics::FontGlyphSet> const &glyphSets);
	Face const& getFace(ui8 size) const;
	Face& getFace(ui8 size);
	std::vector<Face>& faces();

private:
	std::vector<ui8> mSupportedSizes;
	std::vector<Face> mGlyphFaces;
	
	std::optional<uIndex> findSet(ui8 size) const;

};

NS_END
