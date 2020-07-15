#pragma once

#include "TemportalEnginePCH.hpp"

#include "math/Vector.hpp"
#include "graphics/FontAtlas.hpp"
#include "thread/MutexLock.hpp"

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
	 * Locks the object while changing the content and rebuilding geometry.
	 */
	void content(std::string const &str);

#pragma region Properties
private:
	std::weak_ptr<RenderedStringCollection> mpCollection;
	ui8 mFontSize;
	math::Vector2 mPos;
	std::string mContent;
	std::vector<Font::UIVertex> mVerticies;
	std::vector<ui16> mIndicies;
#pragma endregion

#pragma region Renderer Comms
private:
	void initialize(ui8 fontSize, math::Vector2 pos, std::string const &str);
	void rebuildGlyphs();
#pragma endregion

};

class RenderedStringCollection : public std::enable_shared_from_this<RenderedStringCollection>
{

public:
	RenderedStringCollection(std::weak_ptr<class StringRenderer> owner);
	std::weak_ptr<RenderedString> makeString(ui8 fontSize, math::Vector2 pos, std::string const &content);

	std::shared_ptr<class StringRenderer> renderer();
	void rebuildGlyphs();
	void collectGeometry();
	ui32 writeToBuffers(class CommandPool* transientPool, class Buffer* vertexBuffer, class Buffer* indexBuffer);
	bool isDirty() const;

private:
	std::weak_ptr<class StringRenderer> mpOwner;
	std::vector<std::shared_ptr<RenderedString>> mStrings;

	std::vector<Font::UIVertex> mVerticies;
	std::vector<ui16> mIndicies;
	// Mutex lock to prevent geometry from changing while being copied via the render thread
	thread::MutexLock mLock;
	bool mbIsDirty;

	void appendGeometry(std::shared_ptr<RenderedString> str);

};

class StringRenderer : public std::enable_shared_from_this<StringRenderer>
{

public:
	StringRenderer();
	void initialize();

	void setResolution(math::Vector2UInt const &resolution);

	void setFont(std::vector<ui8> const &fontSizes, std::vector<graphics::FontGlyphSet> const &glyphSets);
	graphics::Font& getFont();
	math::Vector2UInt const& getResolution() const;

	std::shared_ptr<RenderedStringCollection> makeExclusiveCollection();
	std::weak_ptr<RenderedString> makeGlobalString(ui8 fontSize, math::Vector2 pos, std::string const &content);

	ui32 writeBuffers(class CommandPool* transientPool, class Buffer* vertexBuffer, class Buffer* indexBuffer);
	bool isDirty() const;

private:
	graphics::Font mFont;
	math::Vector2UInt mResolution;

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
