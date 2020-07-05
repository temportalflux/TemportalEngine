#include "graphics/StringRenderer.hpp"

using namespace graphics;

RenderedString::RenderedString(std::weak_ptr<RenderedStringCollection> collection)
	: mpCollection(collection)
{
}

void RenderedString::initialize(ui8 fontSize, math::Vector2UInt pos, std::string const &str)
{
	this->mFontSize = fontSize;
	this->mPos = pos;
	this->mContent = str;
}

std::string const& RenderedString::content() const
{
	return this->mContent;
}

void RenderedString::content(std::string const &str)
{
	this->mContent = str;
}

std::shared_ptr<RenderedString> RenderedStringCollection::makeString(ui8 fontSize, math::Vector2UInt pos, std::string const &content)
{
	// TODO: Use custom MemoryChunk instead of the global memory
	std::shared_ptr<RenderedString> str = std::make_shared<RenderedString>(this->weak_from_this());
	str->initialize(fontSize, pos, content);
	return str;
}

StringRenderer::StringRenderer()
{
	// TODO: Use custom MemoryChunk instead of the global memory
	this->mpGlobalCollection = std::make_shared<RenderedStringCollection>();
}

void StringRenderer::setFont(std::vector<ui8> const &fontSizes, std::vector<graphics::FontGlyphSet> const &glyphSets)
{
	this->mFont.loadGlyphSets(fontSizes, glyphSets);
}

std::shared_ptr<RenderedStringCollection> StringRenderer::makeExclusiveCollection()
{
	// TODO: Use custom MemoryChunk instead of the global memory
	auto ptr = std::make_shared<RenderedStringCollection>();
	this->mExclusiveCollectionList.push_back(ptr);
	return ptr;
}

std::shared_ptr<RenderedString> StringRenderer::makeGlobalString(ui8 fontSize, math::Vector2UInt pos, std::string const &content)
{
	return this->mpGlobalCollection->makeString(fontSize, pos, content);
}
