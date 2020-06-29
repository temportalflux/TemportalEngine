#include "graphics/FontGlyph.hpp"

#include "cereal/mathVector.hpp"
#include "cereal/list.hpp"

#include <cereal/types/map.hpp>

using namespace graphics;

FontGlyph::FontGlyph(FontGlyph &&other)
{
	*this = std::move(other);
}

FontGlyph& FontGlyph::operator=(FontGlyph &&other)
{
	this->metricsOffset = other.metricsOffset;
	this->metricsSize = other.metricsSize;
	this->advance = other.advance;
	this->bufferSize = other.bufferSize;
	this->buffer = std::move(other.buffer);
	return *this;
}

FontGlyphSet::FontGlyphSet(FontGlyphSet &&other)
{
	*this = std::move(other);
}

FontGlyphSet& FontGlyphSet::operator=(FontGlyphSet &&other)
{
	this->codeToGlyphIdx = std::move(other.codeToGlyphIdx);
	this->glyphs = std::move(other.glyphs);
	return *this;
}

void cereal::save(cereal::JSONOutputArchive &archive, graphics::FontGlyphSet const &value)
{
	assert(false);
}

void cereal::load(cereal::JSONInputArchive &archive, graphics::FontGlyphSet &value)
{
	assert(false);
}

void cereal::save(cereal::PortableBinaryOutputArchive &archive, graphics::FontGlyphSet const &value)
{
	cereal::save(archive, value.codeToGlyphIdx);
	cereal::save(archive, value.glyphs);
}

void cereal::save(cereal::PortableBinaryOutputArchive &archive, graphics::FontGlyph const &value)
{
	archive(value.metricsOffset);
	archive(value.metricsSize);
	archive(value.advance);
	archive(value.bufferSize);
	archive(value.buffer);
}

void cereal::load(cereal::PortableBinaryInputArchive &archive, graphics::FontGlyphSet &value)
{
	cereal::load(archive, value.codeToGlyphIdx);
	cereal::load(archive, value.glyphs);
}

void cereal::load(cereal::PortableBinaryInputArchive &archive, graphics::FontGlyph &value)
{
	archive(value.metricsOffset);
	archive(value.metricsSize);
	archive(value.advance);
	archive(value.bufferSize);
	archive(value.buffer);
}

