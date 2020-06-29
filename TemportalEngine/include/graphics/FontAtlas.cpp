#include "graphics/FontAtlas.hpp"

using namespace graphics;

std::optional<uIndex> Font::findSet(ui8 size) const
{
	auto iter = std::find(this->mSupportedSizes.begin(), this->mSupportedSizes.end(), size);
	return iter != this->mSupportedSizes.end() ? std::make_optional(std::distance(this->mSupportedSizes.begin(), iter)) : std::nullopt;
}

Font::Face& Font::getFace(ui8 size)
{
	auto idxSet = this->findSet(size);
	assert(idxSet);
	return this->mGlyphFaces[*idxSet];
}

void Font::loadGlyphSets(std::vector<ui8> const &fontSizes, std::vector<graphics::FontGlyphSet> const &glyphSets)
{
	assert(fontSizes.size() == glyphSets.size());
	uSize setCount = fontSizes.size();
	this->mSupportedSizes = fontSizes;
	this->mGlyphFaces.resize(setCount);
	for (uIndex idxSet = 0; idxSet < setCount; idxSet++)
	{
		this->mGlyphFaces[idxSet].fontSize = this->mSupportedSizes[idxSet];
		this->mGlyphFaces[idxSet].loadGlyphSet(glyphSets[idxSet]);
	}
}

void Font::Face::loadGlyphSet(FontGlyphSet const &src)
{
	// Copy over glyph metadata
	auto glyphCount = src.glyphs.size();
	this->glyphs.resize(glyphCount);
	this->codeToGlyphIdx = src.codeToGlyphIdx;
	for (auto& [charCode, glyphIdx] : src.codeToGlyphIdx)
	{
		this->glyphs[glyphIdx] = src.glyphs[glyphIdx];
	}
	
	// Determine the atlas size required for the glyphs
	this->atlasSize = this->calculateAtlasLayout();

	// Create the atlas texture

	// Write glyph buffer data to the face's atlas texture
	
}

Font::GlyphSprite& Font::GlyphSprite::operator=(FontGlyph const &other)
{
	this->metricsOffset = other.metricsOffset;
	this->metricsSize = other.metricsSize;
	this->advance = other.advance;
	this->bufferSize = other.bufferSize;
	return *this;
}

// See https://snorristurluson.github.io/TextRenderingWithFreetype/ for reference
math::Vector2UInt Font::Face::measure(std::string str) const
{
	math::Vector2UInt size;
	for (auto& c : str)
	{
		auto& glyph = this->glyphs[c];
		size.x() += glyph.advance;
		size.y() = math::max(size.y(), glyph.atlasOffset.y() + glyph.bufferSize.y());
	}
	return size;
}

math::Vector2UInt Font::Face::calculateAtlasLayout()
{
	// Its very unlikely that the atlas could fit all the glyphs in a size smaller than 256x256
	math::Vector2UInt atlasSize = { 256, 256 };

	// Bumps atlas size to the next power of 2
	atlasSize = { atlasSize.x() << 1, atlasSize.y() << 1 };

	return atlasSize;
}
