#include "graphics/FontAtlas.hpp"

using namespace graphics;

std::optional<uIndex> Font::findSet(ui8 size) const
{
	auto iter = std::find(this->mSupportedSizes.begin(), this->mSupportedSizes.end(), size);
	return iter != this->mSupportedSizes.end() ? std::make_optional(std::distance(this->mSupportedSizes.begin(), iter)) : std::nullopt;
}

Font::Face const& Font::getFace(ui8 size) const
{
	auto idxSet = this->findSet(size);
	assert(idxSet);
	return this->mGlyphFaces[*idxSet];
}

Font::Face& Font::getFace(ui8 size)
{
	auto idxSet = this->findSet(size);
	assert(idxSet);
	return this->mGlyphFaces[*idxSet];
}

std::vector<Font::Face>& Font::faces()
{
	return this->mGlyphFaces;
}

math::Vector2UInt Font::Face::getAtlasSize() const
{
	return this->atlasSize;
}

std::vector<ui8>& Font::Face::getPixelData()
{
	return this->textureData;
}

Font& Font::loadGlyphSets(std::vector<ui8> const &fontSizes, std::vector<graphics::FontGlyphSet> const &glyphSets)
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
	return *this;
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
	this->textureData.resize(this->atlasSize.x() * this->atlasSize.y() * 4); // 4 channels RGBA

	// Write glyph buffer data to the face's atlas texture
	for (auto&[charCode, glyphIdx] : src.codeToGlyphIdx)
	{
		const auto& alphaBuffer = src.glyphs[glyphIdx].buffer;
		if (alphaBuffer.size() > 0)
		{
			this->writeAlphaToTexture(
				this->glyphs[glyphIdx].atlasOffset,
				this->glyphs[glyphIdx].bufferSize,
				alphaBuffer
			);
		}
	}
	
}

Font::GlyphSprite& Font::GlyphSprite::operator=(FontGlyph const &other)
{
	this->bearing = other.bearing;
	this->size = other.size;
	this->advance = other.advance;
	this->bufferSize = other.bufferSize;
	return *this;
}

// See https://snorristurluson.github.io/TextRenderingWithFreetype/ for reference
// See http://freetype.sourceforge.net/freetype2/docs/tutorial/step2.html for unit of measurement reference
std::pair<math::Vector2UInt, math::Vector2Int> Font::Face::measure(std::string const& str) const
{
	math::Vector2UInt size;
	math::Vector2Int offset;
	for (auto& c : str)
	{
		auto& glyph = this->glyphs[c];
		size.x() += glyph.advance;
		size.y() = std::max(size.y(), glyph.size.y());
		offset.y() = std::min(offset.y(), glyph.bearing.y());
	}
	return std::make_pair(size, offset);
}

math::Vector2UInt Font::Face::calculateAtlasLayout()
{
	// Its very unlikely that the atlas could fit all the glyphs in a size smaller than 256x256
	math::Vector2UInt atlasSize = { 256, 256 };

	bool bCanFitAllGlyphs;
	do
	{
		bCanFitAllGlyphs = true;

		math::Vector2UInt rowSize, rowPos;
		for (auto& glyph : this->glyphs)
		{
			if (!(glyph.bufferSize.x() > 0 && glyph.bufferSize.y() > 0)) continue;
			// Row will be exceeded if the glyph is appended to the current row.
			if (rowSize.x() + glyph.bufferSize.x() > atlasSize.x())
			{
				// Atlas height will be exceeded if the row is shifted, atlas needs to be bigger
				if (rowPos.y() + rowSize.y() > atlasSize.y())
				{
					bCanFitAllGlyphs = false;
					// Bumps atlas size to the next power of 2
					atlasSize = { atlasSize.x() << 1, atlasSize.y() << 1 };
					break;
				}
				// Shift the next row down by the largest size recorded
				rowPos.y() += rowSize.y();
				// Reset the size of the row
				rowSize.x(0).y(0);
			}
			glyph.atlasOffset = { rowSize.x(), rowPos.y() };
			rowSize.x() += glyph.bufferSize.x();
			rowSize.y() = math::max(rowSize.y(), glyph.bufferSize.y());
		}
	}
	while (!bCanFitAllGlyphs);

	return atlasSize;
}

void Font::Face::writeAlphaToTexture(math::Vector2UInt const &pos, math::Vector2UInt const &dimensions, std::vector<ui8> const &alpha)
{
	ui32 const channelCountPerPixel = 4; // 4 channels for rgb+a
	for (ui32 x = 0; x < dimensions.x(); ++x) for (ui32 y = 0; y < dimensions.y(); ++y)
	{
		uIndex idxAlpha = y * dimensions.x() + x;
		math::Vector2UInt pixelPos = pos + math::Vector2UInt({ x, y });
		uIndex const idxPixel = pixelPos.y() * this->atlasSize.x() + pixelPos.x();
		uIndex const idxData = idxPixel * channelCountPerPixel;
		this->textureData[idxData + 0] = 0xff;
		this->textureData[idxData + 1] = 0xff;
		this->textureData[idxData + 2] = 0xff;
		this->textureData[idxData + 3] = alpha[idxAlpha];
	}
}

std::optional<Font::GlyphData> Font::Face::getGlyph(char const& charCode) const
{
	auto const& iterGlyph = this->codeToGlyphIdx.find((ui32)charCode);
	if (iterGlyph == this->codeToGlyphIdx.end()) { return std::nullopt; }
	auto const& sprite = this->glyphs[iterGlyph->second];
	math::Vector2 const atlasSize = this->atlasSize.toFloat();

	GlyphData data;
	data.bearing = sprite.bearing.toFloat();
	data.size = sprite.size.toFloat();
	data.advance = f32(sprite.advance);
	data.uvPos = sprite.atlasOffset.toFloat() / atlasSize;
	data.uvSize = sprite.size.toFloat() / atlasSize;

	return data;
}

i32 Font::Face::appendGlyph(
	ui32 const charCode,
	math::Vector2 const &rootPos, math::Vector2 const &resolution, i32 const &advance,
	std::vector<UIVertex> &verticies, std::vector<ui16> &indicies
) const
{
	auto const& vertexPreCount = (ui16)verticies.size();
	auto const& iterGlyph = this->codeToGlyphIdx.find(charCode);
	if (iterGlyph == this->codeToGlyphIdx.end()) { return 0; }
	auto const& glyph = this->glyphs[iterGlyph->second];
	math::Vector4 const posScreen = { rootPos.x(), rootPos.y(), (f32)advance / resolution.x(), 0 };
	math::Vector2 const atlasSize = this->atlasSize.toFloat();
	math::Vector2 const posAtlas = glyph.atlasOffset.toFloat() / atlasSize;

	math::Vector4 const bearingScreen = (glyph.bearing.toFloat() / resolution).createSubvector<4>(2);
	math::Vector2 const sizeScreen = glyph.size.toFloat() / resolution;
	math::Vector2 const sizeAtlas = glyph.size.toFloat() / atlasSize;

	const auto pushVertex = [&](ui16 i, math::Vector2 const p) -> ui16 {
		// Vertex Position
		// pos: move pen to origin of glyph (combines pos of string with the advance of previous characters)
		// bearing: offset to the top-left corner
		// size * p: offset to the corner of the glyph this vertex is representing
		math::Vector4 const pos = posScreen + bearingScreen + (sizeScreen * p).createSubvector<4>(2);
		// TexCoord
		// pos: tex coord starts at the glyphs position in the atlas
		// size * p: tex coord relative to the glyph is based on the vertex it is representing
		math::Vector2 const texCoord = posAtlas + (sizeAtlas * p);
		verticies.push_back({ pos, texCoord });
		return i + vertexPreCount;
	};

	// only push vertex data if the glyph is not whitespace
	if (glyph.bufferSize.x() > 0 && glyph.bufferSize.y() > 0)
	{
		ui16 const idxTL = pushVertex(0, { 0, 0 });
		ui16 const idxTR = pushVertex(1, { 1, 0 });
		ui16 const idxBR = pushVertex(2, { 1, 1 });
		ui16 const idxBL = pushVertex(3, { 0, 1 });
		indicies.push_back(idxTL);
		indicies.push_back(idxTR);
		indicies.push_back(idxBR);
		indicies.push_back(idxBR);
		indicies.push_back(idxBL);
		indicies.push_back(idxTL);
	}

	return glyph.advance;
}
