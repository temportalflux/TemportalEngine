#include "graphics/StringRenderer.hpp"

#include "graphics/Buffer.hpp"

using namespace graphics;

RenderedString::RenderedString(std::weak_ptr<RenderedStringCollection> collection)
	: mpCollection(collection)
{
}

void RenderedString::initialize(ui8 fontSize, math::Vector2 pos, std::string const &str)
{
	this->mFontSize = fontSize;
	this->mPos = pos;
	this->mContent = str;
	this->rebuildGlyphs();
}

std::string const& RenderedString::content() const
{
	return this->mContent;
}

void RenderedString::content(std::string const &str)
{
	this->mContent = str;
}

void RenderedString::rebuildGlyphs()
{
	auto renderer = this->mpCollection.lock()->renderer();
	auto const& resolution = renderer->getResolution().toFloat();
	if (resolution.sum() <= 0) return;

	const auto& charCount = this->content().size();

	this->mVerticies.clear();
	this->mVerticies.reserve(charCount * 4);
	this->mIndicies.clear();
	this->mIndicies.reserve(charCount * 6);
	
	auto& face = renderer->getFont().getFace(this->mFontSize);
	i32 advance = 0;
	auto const measurement = face.measure(this->content());
	auto const offset = measurement.second;
	math::Vector2 const pos = this->mPos - (offset.toFloat() / resolution);
	for (const auto& c : this->content())
	{
		advance += face.appendGlyph((ui32)c, pos, resolution, advance, this->mVerticies, this->mIndicies);
	}
}

RenderedStringCollection::RenderedStringCollection(std::weak_ptr<StringRenderer> owner)
{
	this->mpOwner = owner;
}

std::shared_ptr<StringRenderer> RenderedStringCollection::renderer()
{
	return this->mpOwner.lock();
}

std::weak_ptr<RenderedString> RenderedStringCollection::makeString(ui8 fontSize, math::Vector2 pos, std::string const &content)
{
	// TODO: Use custom MemoryChunk instead of the global memory
	std::shared_ptr<RenderedString> str = std::make_shared<RenderedString>(this->weak_from_this());
	this->mStrings.push_back(str);
	str->initialize(fontSize, pos, content);
	return str;
}

void RenderedStringCollection::rebuildGlyphs()
{
	for (auto& str : this->mStrings)
	{
		str->rebuildGlyphs();
	}
}

ui32 RenderedStringCollection::writeToBuffers(GameRenderer* renderer, Buffer* vertexBuffer, Buffer* indexBuffer)
{
	uSize offsetVertex = 0, offsetIndex = 0;
	ui32 indexCount = 0;
	for (auto& str : this->mStrings)
	{
		uSize vertexDataSize = str->mVerticies.size() * sizeof(Font::UIVertex);
		vertexBuffer->writeBuffer(renderer, offsetVertex, str->mVerticies.data(), vertexDataSize);
		offsetVertex += vertexDataSize;

		indexCount += (ui32)str->mIndicies.size();
		uSize indexDataSize = str->mIndicies.size() * sizeof(ui16);
		indexBuffer->writeBuffer(renderer, offsetIndex, str->mIndicies.data(), indexDataSize);
		offsetIndex += indexDataSize;
	}
	return indexCount;
}

StringRenderer::StringRenderer()
{
}

void StringRenderer::initialize()
{
	// TODO: Use custom MemoryChunk instead of the global memory
	this->mpGlobalCollection = std::make_shared<RenderedStringCollection>(this->weak_from_this());
}

void StringRenderer::setResolution(math::Vector2UInt const &resolution)
{
	this->mResolution = resolution;
	this->mpGlobalCollection->rebuildGlyphs();
	for (auto& collection : this->mExclusiveCollectionList)
	{
		collection->rebuildGlyphs();
	}
}

math::Vector2UInt const& StringRenderer::getResolution() const
{
	return this->mResolution;
}

void StringRenderer::setFont(std::vector<ui8> const &fontSizes, std::vector<graphics::FontGlyphSet> const &glyphSets)
{
	this->mFont.loadGlyphSets(fontSizes, glyphSets);
}

graphics::Font& StringRenderer::getFont()
{
	return this->mFont;
}

std::shared_ptr<RenderedStringCollection> StringRenderer::makeExclusiveCollection()
{
	// TODO: Use custom MemoryChunk instead of the global memory
	auto ptr = std::make_shared<RenderedStringCollection>(this->weak_from_this());
	this->mExclusiveCollectionList.push_back(ptr);
	return ptr;
}

std::weak_ptr<RenderedString> StringRenderer::makeGlobalString(ui8 fontSize, math::Vector2 pos, std::string const &content)
{
	return this->mpGlobalCollection->makeString(fontSize, pos, content);
}

ui32 StringRenderer::writeBuffers(GameRenderer* renderer, Buffer* vertexBuffer, Buffer* indexBuffer)
{
	auto const& maxCharCount = indexBuffer->getSize() / (sizeof(ui16) * /*indicies per char*/ 6);
	return this->mpGlobalCollection->writeToBuffers(renderer, vertexBuffer, indexBuffer);
}
