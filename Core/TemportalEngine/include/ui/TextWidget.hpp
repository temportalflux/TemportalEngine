#pragma once

#include "ui/Widget.hpp"

#include "graphics/AttributeBinding.hpp"
#include "graphics/Buffer.hpp"
#include "graphics/FontAtlas.hpp"

FORWARD_DEF(NS_GRAPHICS, class Font);

NS_UI
class FontOwner;

class Text : public ui::Widget
{

public:

	Text();
	Text(Text const& other) = delete;
	Text(Text&& other);
	Text& operator=(Text &&other);
	virtual ~Text();

	static graphics::AttributeBinding binding();

	Text& setFontOwner(std::weak_ptr<ui::FontOwner> fontOwner);

	void setDevice(std::weak_ptr<graphics::GraphicsDevice> device) override;

	Text& setParent(std::weak_ptr<ui::Widget> parent);
	Text& setAnchor(math::Vector2 const& anchor);
	Text& setPivot(math::Vector2 const& pivot);
	Text& setPosition(math::Vector2Int const& points);
	Text& setSize(math::Vector2UInt const& points);
	Text& setFillWidth(bool bFill);
	Text& setFillHeight(bool bFill);
	Text& setZLayer(ui32 z);

	Text& setFont(std::string const& fontId);
	Text& setFontSize(ui16 fontSize);
	Text& setMaxContentLength(ui32 length);
	ui32 maxContentLength() const { return this->mUncommitted.maxLength; }
	Text& setContent(std::string const& content, bool isMaxLength = false);
	Text& operator=(std::string const& content);
	std::string const& string() const { return this->mUncommitted.content; }
	Text& setThickness(f32 characterWidth);
	Text& setEdgeWidth(f32 charEdgeWidth);
	
	math::Vector2 getSizeOnScreen() const override;

	void releaseGraphics();
	Text& commit();
	void record(graphics::Command *command) override;

protected:

	struct Vertex
	{
		/**
		 * <x,y> The position of a text/string vertex.
		 * <z, w> The width and edge distance of the character in sdf.
		 */
		math::Vector4 positionAndWidthEdge;
		/**
		 * The coordinate of the glyph vertex in the font atlas allocated for a given `RegisteredFont`.
		 */
		math::Vector2Padded texCoord;
	};

	std::vector<Vertex> mVertices;
	std::vector<ui16> mIndices;
	ui32 mCommittedIndexCount;

	graphics::Font const* getFont() const;
	math::Vector4 widthEdge() const;
	ui16 pushVertex(Vertex const& v);
	void pushGlyph(math::Vector2 &cursorPos, f32 fontHeight, graphics::Font::GlyphSprite const& glyph);
	virtual void prePushCharacter(uIndex charIndex, math::Vector2 &cursorPos, f32 fontHeight);
	virtual void onPushedAllCharacters(math::Vector2 &cursorPos, f32 fontHeight);
	virtual ui32 desiredCharacterCount() const;

private:

	std::weak_ptr<ui::FontOwner> mpFontOwner;

	struct Configuration
	{
		std::string fontId;
		ui16 fontSize; // in points
		std::string content;
		f32 thickness;
		f32 edgeWidth;
		ui32 maxLength;
	};
	Configuration mUncommitted, mCommitted;
	ui32 mBufferCharacterCount;

	graphics::Buffer mVertexBuffer;
	graphics::Buffer mIndexBuffer;

	virtual void populateBufferData();

};

NS_END
