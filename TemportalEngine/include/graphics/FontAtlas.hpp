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

	struct UIVertex
	{
		math::Vector2 position;
		math::Vector4 color;
		math::Vector2 texCoord;
	};

public:
	class Face
	{
		friend class Font;
	public:
		math::Vector2UInt measure(std::string str) const;
		graphics::ImageSampler& sampler();
		graphics::Image& image();
		graphics::ImageView& view();
		math::Vector2UInt getAtlasSize() const;
		std::vector<ui8>& getPixelData();
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
