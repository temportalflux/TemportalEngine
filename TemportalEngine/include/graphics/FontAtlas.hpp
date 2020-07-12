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

	class Face
	{
		friend class Font;
	public:
		std::pair<math::Vector2UInt, math::Vector2Int> measure(std::string const& str) const;
		graphics::ImageSampler& sampler();
		graphics::Image& image();
		graphics::ImageView& view();
		math::Vector2UInt getAtlasSize() const;
		std::vector<ui8>& getPixelData();
		i32 appendGlyph(
			ui32 const charCode,
			math::Vector2 const &rootPos, math::Vector2 const &resolution,
			i32 const &advance,
			std::vector<UIVertex> &verticies, std::vector<ui16> &indicies
		) const;
	private:
		ui8 fontSize;
		std::unordered_map<ui32, ui32> codeToGlyphIdx;
		std::vector<GlyphSprite> glyphs;
		math::Vector2UInt atlasSize;
		std::vector<ui8> textureData;
		graphics::Image mImage;
		graphics::ImageSampler mSampler;
		graphics::ImageView mView;
		void loadGlyphSet(FontGlyphSet const &src);
		// Determines the glyph offsets and atlas size
		math::Vector2UInt calculateAtlasLayout();
		void writeAlphaToTexture(math::Vector2UInt const &pos, math::Vector2UInt const &dimensions, std::vector<ui8> const &alpha);
		void invalidate();

		struct PositionedString
		{
			math::Vector2Int pos;
			std::string content;
			std::vector<UIVertex> verticies;
		};
		std::unordered_map<std::string, PositionedString> mRenderingText;
		void setText(std::string const key, math::Vector2Int pos, std::string const content);
	};

	Font& loadGlyphSets(std::vector<ui8> const &fontSizes, std::vector<graphics::FontGlyphSet> const &glyphSets);
	Face& getFace(ui8 size);
	std::vector<Face>& faces();

	void setText(std::string const key, ui8 fontSize, math::Vector2Int pos, std::string const content);

	void invalidate();

private:
	std::vector<ui8> mSupportedSizes;
	std::vector<Face> mGlyphFaces;
	
	std::optional<uIndex> findSet(ui8 size) const;

};

NS_END
