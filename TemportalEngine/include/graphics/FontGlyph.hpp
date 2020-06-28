#pragma once

#include "TemportalEnginePCH.hpp"

#include "math/Vector.hpp"

#include <cereal/archives/json.hpp>
#include <cereal/archives/portable_binary.hpp>
#include <cereal/cereal.hpp>

NS_GRAPHICS

struct FontGlyph
{
	math::Vector2Int offset;
	math::Vector2Int size;
	i32 advance;
	math::Vector2UInt bufferSize;
	std::vector<ui8> buffer;

	FontGlyph() = default;
	FontGlyph(FontGlyph &&other);
	FontGlyph& operator=(FontGlyph &&other);

};

struct FontGlyphSet
{
	std::unordered_map<char, FontGlyph> supportedCharacters;

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
