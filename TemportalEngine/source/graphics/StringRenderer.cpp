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
	this->rebuildGlyphs();
	this->mpCollection.lock()->collectGeometry();
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
	
	this->mLock.lock();
	this->appendGeometry(str);
	this->mLock.unlock();

	return str;
}

void RenderedStringCollection::rebuildGlyphs()
{
	for (auto& str : this->mStrings)
	{
		str->rebuildGlyphs();
	}
	this->collectGeometry();
}

void RenderedStringCollection::appendGeometry(std::shared_ptr<RenderedString> str)
{
	ui16 preVertexCount = (ui16)this->mVerticies.size();
	this->mVerticies.insert(this->mVerticies.end(), str->mVerticies.begin(), str->mVerticies.end());
	std::transform(
		str->mIndicies.cbegin(), str->mIndicies.cend(),
		std::back_inserter(this->mIndicies), [preVertexCount](auto const& idx) -> ui16 {
			return preVertexCount + idx;
		}
	);
}

void RenderedStringCollection::collectGeometry()
{
	this->mLock.lock();

	this->mVerticies.clear();
	this->mIndicies.clear();
	for (auto& str : this->mStrings)
	{
		this->appendGeometry(str);
	}

	this->mbIsDirty = true;

	this->mLock.unlock();
}

ui32 RenderedStringCollection::writeToBuffers(GameRenderer* renderer, Buffer* vertexBuffer, Buffer* indexBuffer)
{
	this->mLock.lock();
	
	ui32 indexCount = (ui32)this->mIndicies.size();
	uSize vertexDataSize = this->mVerticies.size() * sizeof(Font::UIVertex);
	vertexBuffer->writeBuffer(renderer, 0, this->mVerticies.data(), vertexDataSize);
	uSize indexDataSize = indexCount * sizeof(ui16);
	indexBuffer->writeBuffer(renderer, 0, this->mIndicies.data(), indexDataSize, true);

	this->mbIsDirty = false;

	this->mLock.unlock();
	return indexCount;
}

bool RenderedStringCollection::isDirty() const
{
	return this->mbIsDirty;
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

bool StringRenderer::isDirty() const
{
	return this->mpGlobalCollection->isDirty();
}
