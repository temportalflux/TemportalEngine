#pragma once

#include "TemportalEnginePCH.hpp"

#include "graphics/Descriptor.hpp"
#include "graphics/Image.hpp"
#include "graphics/ImageSampler.hpp"
#include "graphics/ImageView.hpp"

NS_GRAPHICS

class Font
{

public:
	struct GlyphSprite
	{
		math::Vector2 atlasPos;
		math::Vector2 atlasSize;

		f32 pointBasisRatio;
		math::Vector2 size;
		math::Vector2 bearing;
		f32 advance;
	};

	Font();
	Font(Font &&other);
	Font& operator=(Font &&other);
	~Font();

	void setSampler(graphics::ImageSampler *sampler);

	math::Vector2UInt& atlasSize();
	std::vector<ui8>& atlasPixels();
	graphics::DescriptorSet const& descriptorSet() const;
	graphics::DescriptorSet& descriptorSet();
	void addGlyph(char code, GlyphSprite&& sprite);

	void setDevice(std::weak_ptr<graphics::GraphicsDevice> device);
	void initializeImage(graphics::CommandPool* transientPool);

	/**
	 * Measures a given string in the font.
	 * x: total width of the string
	 * y: total height of the string
	 * z: the distance from the cursor y-pos to the top of the string
	 */
	math::Vector<f32, 3> measure(std::string const& str) const;

	GlyphSprite const& operator[](char const& code) const;

private:
	math::Vector2UInt mAtlasSize;
	std::vector<ui8> mAtlasPixels;

	std::map<char, uIndex> mCharToGlyphIdx;
	std::vector<GlyphSprite> mGlyphSprites;
	
	/**
	 * The graphics image for the signed-distance-field atlas.
	 */
	graphics::Image mImage;
	graphics::ImageView mView;
	graphics::ImageSampler *mpSampler;

	/**
	 * The descriptor set for the font SDF atlas.
	 */
	graphics::DescriptorSet mDescriptorSet;

};

NS_END
