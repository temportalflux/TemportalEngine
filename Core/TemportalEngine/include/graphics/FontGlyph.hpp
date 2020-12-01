#pragma once

#include "TemportalEnginePCH.hpp"

#include "math/Vector.hpp"

#include <limits>
#include <cereal/archives/json.hpp>
#include <cereal/archives/portable_binary.hpp>
#include <cereal/cereal.hpp>

NS_GRAPHICS

struct FontGlyphMeta
{
	math::Vector2Int bearing;
	math::Vector2UInt size;
	ui32 advance;
	
	// The width and rows in the bitmap
	math::Vector2UInt bufferSize;

	operator bool() const { return this->advance > 0; }
};

struct FontGlyph : public FontGlyphMeta
{
	std::vector<ui8> buffer;

	FontGlyph() = default;
	FontGlyph(FontGlyph &&other);
	FontGlyph& operator=(FontGlyph &&other);

};

struct FontGlyphSet
{
	std::unordered_map<ui32, ui32> codeToGlyphIdx;
	std::vector<FontGlyph> glyphs;

	FontGlyphSet() = default;
	FontGlyphSet(FontGlyphSet &&other);
	FontGlyphSet& operator=(FontGlyphSet &&other);
};

NS_END

namespace cereal
{
	void save(cereal::JSONOutputArchive &archive, graphics::FontGlyphSet const &value);
	void load(cereal::JSONInputArchive &archive, graphics::FontGlyphSet &value);
	void save(cereal::PortableBinaryOutputArchive &archive, graphics::FontGlyphSet const &value);
	void load(cereal::PortableBinaryInputArchive &archive, graphics::FontGlyphSet &value);
	void save(cereal::PortableBinaryOutputArchive &archive, graphics::FontGlyph const &value);
	void load(cereal::PortableBinaryInputArchive &archive, graphics::FontGlyph &value);
}
