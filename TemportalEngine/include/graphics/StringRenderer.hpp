#pragma once

#include "TemportalEnginePCH.hpp"

#include "math/Vector.hpp"
#include "graphics/FontAtlas.hpp"

NS_GRAPHICS
class StringRenderer;
class RenderedStringCollection;

class RenderedString
{
	friend class RenderedStringCollection;

public:
	RenderedString(std::weak_ptr<RenderedStringCollection> mpCollection);

	/**
	 * Returns a constant reference to the string currently available for render.
	 * Cannot be used to modify the string, use the setter `content(std::string)`.
	 */
	std::string const& content() const;
	/**
	 * Sets the content of the rendered string, and updating the vertex data accordingly.
	 */
	void content(std::string const &str);

#pragma region Properties
private:
	std::weak_ptr<RenderedStringCollection> mpCollection;
	ui8 mFontSize;
	math::Vector2UInt mPos;
	std::string mContent;
#pragma endregion

#pragma region Renderer Comms
private:
	void initialize(ui8 fontSize, math::Vector2UInt pos, std::string const &str);
#pragma endregion

};

class RenderedStringCollection : public std::enable_shared_from_this<RenderedStringCollection>
{

public:
	std::shared_ptr<RenderedString> makeString(ui8 fontSize, math::Vector2UInt pos, std::string const &content);

};

class StringRenderer : public std::enable_shared_from_this<StringRenderer>
{

public:
	StringRenderer();

	void setFont(std::vector<ui8> const &fontSizes, std::vector<graphics::FontGlyphSet> const &glyphSets);

	std::shared_ptr<RenderedStringCollection> makeExclusiveCollection();
	std::shared_ptr<RenderedString> makeGlobalString(ui8 fontSize, math::Vector2UInt pos, std::string const &content);

private:
	graphics::Font mFont;

	/**
	 * A collection of RenderedStrings which is always visible.
	 */
	std::shared_ptr<RenderedStringCollection> mpGlobalCollection;
	/**
	 * A set of exclusive string collections, in which only one is enabled at a given time.
	 */
	std::vector<std::shared_ptr<RenderedStringCollection>> mExclusiveCollectionList;
	/**
	 * The currently active exclusive string collection.
	 * Owned pointer exists in `mExclusiveCollectionList`.
	 */
	std::weak_ptr<RenderedStringCollection> mpActiveExclusiveCollection;

};

NS_END
