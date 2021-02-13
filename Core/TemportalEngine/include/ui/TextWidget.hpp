#pragma once

#include "ui/Widget.hpp"

#include "graphics/AttributeBinding.hpp"
#include "graphics/Buffer.hpp"
#include "graphics/FontAtlas.hpp"

FORWARD_DEF(NS_GRAPHICS, class Font);
FORWARD_DEF(NS_GRAPHICS, class WidgetRenderer);

NS_UI

class Text : public ui::Widget
{

public:

	Text();
	Text(Text const& other) = delete;
	Text(Text&& other);
	Text& operator=(Text &&other);
	virtual ~Text();

	static graphics::AttributeBinding binding();

	Text& setFontOwner(std::weak_ptr<ui::WidgetRenderer> fontOwner);

	void setDevice(std::weak_ptr<graphics::GraphicsDevice> device) override;

	Text& setParent(std::weak_ptr<ui::Widget> parent);
	Text& setParentFlag(EParentFlags flag, bool bEnabled);
	Text& setAnchor(math::Vector2 const& anchor);
	Text& setPivot(math::Vector2 const& pivot);
	Text& setPosition(math::Vector2Int const& points);
	Text& setSize(math::Vector2UInt const& points);
	Text& setFillWidth(bool bFill);
	Text& setFillHeight(bool bFill);
	Text& setZLayer(ui32 z);

	Text& setFont(std::string const& fontId);
	Text& setFontSize(ui16 fontSize);
	Text& setThickness(f32 characterWidth);
	Text& setEdgeWidth(f32 charEdgeWidth);
	Text& setMaxContentLength(ui32 length);
	ui32 maxContentLength() const;

	Text& setContent(std::string const& content);
	Text& startContent();
	Text& addSegment(std::string const& content);
	Text& setSegmentColor(math::Color const& color);
	Text& finishContent();
	
	math::Vector2 getSizeOnScreen() const override;

	void releaseGraphics();
	Text& commit();
	void record(graphics::Command *command) override;

protected:

	struct Segment
	{
		std::string content;
		math::Color color;
	};

	using TDrawGlyph = std::function<void(
		math::Vector2 const& pos, graphics::Font::GlyphSprite const& glyph,
		Segment const& segment
	)>;

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
		math::Vector4 color;
	};

	std::vector<Vertex> mVertices;
	std::vector<ui16> mIndices;
	ui32 mCommittedIndexCount;

	std::vector<Segment>& uncommittedSegments();
	ui32& uncommittedContentLength();
	graphics::Font const* getFont() const;
	math::Vector4 widthEdge() const;
	ui16 pushVertex(Vertex const& v);
	virtual ui32 desiredCharacterCount() const;
	virtual Segment const& segmentAt(uIndex idxSegment, uIndex idxSegmentChar, uIndex idxTotalChar) const;
	virtual char charAt(uIndex idxSegment, uIndex idxSegmentChar, uIndex idxTotalChar) const;
	virtual bool incrementChar(uIndex &idxSegment, uIndex &idxSegmentChar, uIndex idxTotalChar) const;

private:

	std::weak_ptr<ui::WidgetRenderer> mpFontOwner;

	struct Configuration
	{
		std::string fontId;
		ui16 fontSize; // in points
		std::vector<Segment> segments;
		ui32 totalContentLength;
		f32 thickness;
		f32 edgeWidth;
		ui32 maxLength;
	};
	Configuration mUncommitted, mCommitted;
	ui32 mBufferCharacterCount;

	graphics::Buffer mOldVBuffer, mOldIBuffer;
	ui8 mFramesSinceBufferRecreation;

	graphics::Buffer mVertexBuffer;
	graphics::Buffer mIndexBuffer;

	void populateBufferData();
	f32 getFontHeight() const;
	math::Vector2 writeGlyphs(
		math::Vector2 const& offset, math::Vector2 &cursorPos, math::Vector2 const& maxBounds,
		TDrawGlyph draw
	) const;

	math::Vector2 glyphToFontSize(
		graphics::Font::GlyphSprite const& glyph, f32 fontHeight
	) const;
	void pushGlyph(
		math::Vector2 const& cursorPos,
		graphics::Font::GlyphSprite const& glyph,
		Segment const& segment
	);
	f32 toScreenHeight(i32 points) const;

};

NS_END
