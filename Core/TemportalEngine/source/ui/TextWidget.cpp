#include "ui/TextWidget.hpp"

#include "graphics/Command.hpp"
#include "math/Common.hpp"
#include "ui/WidgetRenderer.hpp"

using namespace ui;

Text::Text() : ui::Widget()
{
	this->mUncommitted.thickness = 0.5f;
	this->mUncommitted.edgeWidth = 0.1f;
	this->mUncommitted.maxLength = 0;
	this->mUncommitted.totalContentLength = 0;
	this->mVertexBuffer.setUsage(vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eVertexBuffer, graphics::MemoryUsage::eGPUOnly);
	this->mIndexBuffer.setUsage(vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eIndexBuffer, graphics::MemoryUsage::eGPUOnly);
}

Text::Text(Text&& other) { *this = std::move(other); }

Text& Text::operator=(Text &&other)
{
	this->mUncommitted = other.mUncommitted;
	this->mVertices = other.mVertices;
	this->mIndices = other.mIndices;
	this->mVertexBuffer = std::move(other.mVertexBuffer);
	this->mIndexBuffer = std::move(other.mIndexBuffer);
	return *this;
}

Text::~Text()
{
	this->releaseGraphics();
}

graphics::AttributeBinding Text::binding()
{
	return graphics::AttributeBinding(graphics::AttributeBinding::Rate::eVertex)
		.setStructType<Text::Vertex>()
		.addAttribute({ 0, /*vec4*/ (ui32)vk::Format::eR32G32B32A32Sfloat, offsetof(Text::Vertex, positionAndWidthEdge) })
		.addAttribute({ 1, /*vec2*/ (ui32)vk::Format::eR32G32Sfloat, offsetof(Text::Vertex, texCoord) })
		.addAttribute({ 2, /*vec4*/ (ui32)vk::Format::eR32G32B32A32Sfloat, offsetof(Text::Vertex, color) })
		;
}

Text& Text::setFontOwner(std::weak_ptr<ui::FontOwner> fontOwner)
{
	this->mpFontOwner = fontOwner;
	return *this;
}

void Text::setDevice(std::weak_ptr<graphics::GraphicsDevice> device)
{
	Widget::setDevice(device);
	this->mVertexBuffer.setDevice(device);
	this->mIndexBuffer.setDevice(device);
}

Text& Text::setParent(std::weak_ptr<ui::Widget> parent) { Widget::setParent(parent); return *this; }
Text& Text::setParentFlag(EParentFlags flag, bool bEnabled) { Widget::setParentFlag(flag, bEnabled); return *this; }
Text& Text::setAnchor(math::Vector2 const& anchor) { Widget::setAnchor(anchor); return *this; }
Text& Text::setPivot(math::Vector2 const& pivot) { Widget::setPivot(pivot); return *this; }
Text& Text::setPosition(math::Vector2Int const& points) { Widget::setPosition(points); return *this; }
Text& Text::setSize(math::Vector2UInt const& points) { Widget::setSize(points); return *this; }
Text& Text::setFillWidth(bool bFill) { Widget::setFillWidth(bFill); return *this; }
Text& Text::setFillHeight(bool bFill) { Widget::setFillHeight(bFill); return *this; }
Text& Text::setZLayer(ui32 z) { Widget::setZLayer(z); return *this; }

Text& Text::setFont(std::string const& fontId)
{
	this->lock();
	this->mUncommitted.fontId = fontId;
	this->markDirty();
	this->unlock();
	return *this;
}

Text& Text::setFontSize(ui16 fontSize)
{
	this->lock();
	this->mUncommitted.fontSize = fontSize;
	this->markDirty();
	this->unlock();
	return *this;
}

Text& Text::setThickness(f32 characterWidth)
{
	this->lock();
	this->mUncommitted.thickness = characterWidth;
	this->markDirty();
	this->unlock();
	return *this;
}

Text& Text::setEdgeWidth(f32 charEdgeWidth)
{
	this->lock();
	this->mUncommitted.edgeWidth = charEdgeWidth;
	this->markDirty();
	this->unlock();
	return *this;
}

Text& Text::setMaxContentLength(ui32 length)
{
	this->lock();
	this->mUncommitted.maxLength = length;
	this->markDirty();
	this->unlock();
	return *this;
}

ui32 Text::maxContentLength() const { return this->mUncommitted.maxLength; }

Text& Text::setContent(std::string const& content)
{
	this->startContent();
	this->addSegment(content);
	this->finishContent();
	return *this;
}

Text& Text::startContent()
{
	this->lock();
	this->mUncommitted.segments.clear();
	this->mUncommitted.totalContentLength = 0;
	return *this;
}

Text& Text::addSegment(std::string const& content)
{
	auto segment = Segment();
	segment.content = content;
	segment.color = math::Color(1);
	this->mUncommitted.segments.push_back(segment);
	this->mUncommitted.totalContentLength += (ui32)content.length();
	return *this;
}

Text& Text::setSegmentColor(math::Color const& color)
{
	auto& segments = this->mUncommitted.segments;
	assert(segments.size() > 0);
	segments[segments.size() - 1].color = color;
	return *this;
}

Text& Text::finishContent()
{
	this->mUncommitted.maxLength = math::max(
		this->mUncommitted.maxLength, this->mUncommitted.totalContentLength
	);
	this->markDirty();
	this->unlock();
	return *this;
}

std::vector<Text::Segment>& Text::uncommittedSegments() { return this->mUncommitted.segments; }
ui32& Text::uncommittedContentLength() { return this->mUncommitted.totalContentLength; }

void Text::releaseGraphics()
{
	this->mVertexBuffer.invalidate();
	this->mIndexBuffer.invalidate();
}

Text& Text::commit()
{
	this->lock();
	// TODO: Check if there are actually any changes before updating buffers (this needs to include resolution)

	this->mVertices.clear();
	this->mIndices.clear();
	this->mVertices.reserve(this->mUncommitted.totalContentLength * 4);
	this->mIndices.reserve(this->mUncommitted.totalContentLength * 6);

	if (this->desiredCharacterCount() != this->mBufferCharacterCount)
	{
		this->mBufferCharacterCount = this->desiredCharacterCount();
		
		this->mOldVBuffer = std::move(this->mVertexBuffer);
		this->mOldIBuffer = std::move(this->mIndexBuffer);
		this->mFramesSinceBufferRecreation = 0;
		
		this->mVertexBuffer
			.setSize(this->mBufferCharacterCount * 4 * sizeof(Vertex))
			.create();
		this->mIndexBuffer
			.setSize(this->mBufferCharacterCount * 6 * sizeof(ui16))
			.create();
	}

	this->populateBufferData();

	// TODO: Can optimize by only writing changed regions (based on the diff between content and committed content, and if the font has changed)
	if (this->mVertices.size() > 0)
	{
		assert(sizeof(this->mVertices) <= this->mVertexBuffer.getSize());
		this->mVertexBuffer.writeBuffer(this->renderer()->getTransientPool(), 0, this->mVertices);
	}
	if (this->mIndices.size() > 0)
	{
		assert(sizeof(this->mIndices) <= this->mIndexBuffer.getSize());
		this->mIndexBuffer.writeBuffer(this->renderer()->getTransientPool(), 0, this->mIndices);
	}

	this->mCommitted = this->mUncommitted;
	this->mCommittedIndexCount = (ui32)this->mIndices.size();
	
	this->markClean();
	this->unlock();
	return *this;
}

ui32 Text::desiredCharacterCount() const
{
	return math::max(this->mUncommitted.maxLength, this->mUncommitted.totalContentLength);
}

graphics::Font const* Text::getFont() const
{
	return &this->mpFontOwner.lock()->getFont(this->mUncommitted.fontId);
}

math::Vector2 Text::getSizeOnScreen() const
{	
	auto cursorPos = math::Vector2::ZERO;
	auto generatedBounds = this->writeGlyphs(
		math::Vector2::ZERO, cursorPos,
		Widget::getSizeOnScreen(), nullptr
	);
	return generatedBounds;
}

math::Vector4 Text::widthEdge() const
{
	return math::Vector2({ this->mUncommitted.thickness, this->mUncommitted.edgeWidth }).createSubvector<4>(2);
}

ui16 Text::pushVertex(Vertex const& v)
{
	uIndex const idxVertex = this->mVertices.size();
	this->mVertices.push_back(v);
	return (ui16)idxVertex;
}

f32 Text::toScreenHeight(i32 points) const
{
	return this->resolution().pointsToScreenSpace({ 0, points }).y();
}

void Text::populateBufferData()
{
	auto const& topLeft = this->getTopLeftPositionOnScreen();
	auto cursorPos = math::Vector2::ZERO;
	this->writeGlyphs(
		topLeft, cursorPos, Widget::getSizeOnScreen(),
		std::bind(
			&Text::pushGlyph, this,
			std::placeholders::_1, std::placeholders::_2, std::placeholders::_3
		)
	);
}

f32 Text::getFontHeight() const
{
	return this->toScreenHeight(this->mUncommitted.fontSize);
}

math::Vector2 Text::writeGlyphs(
	math::Vector2 const& offset, math::Vector2 &cursorPos, math::Vector2 const& maxBounds,
	TDrawGlyph drawer
) const
{
	f32 fontHeight = this->getFontHeight();
	f32 lineHeight = fontHeight + this->toScreenHeight(5);

	graphics::Font const& font = *this->getFont();
	auto bounds = math::Vector2({ 0, lineHeight });
	
	auto pushChar = [&](char c, Segment const& segment)
	{
		if (c == '\n' || c == '\r')
		{
			cursorPos.y() += lineHeight;
			cursorPos.x() = 0;
			bounds.y() += lineHeight;
			return true;
		}
		if (!font.contains(c)) c = '?';

		auto const& glyph = font[c];
		auto toFontSize = this->glyphToFontSize(glyph, fontHeight);
		auto glyphSize = glyph.size.toFloat() * toFontSize;
		if (maxBounds.x() > 0 && cursorPos.x() + glyphSize.x() > maxBounds.x())
		{
			cursorPos.y() += lineHeight;
			cursorPos.x() = 0;
			bounds.y() += lineHeight;
		}
		if (maxBounds.y() > 0 && cursorPos.y() + glyphSize.y() > maxBounds.y())
		{
			return false;
		}
		if (drawer) drawer(offset + cursorPos, glyph, segment);
		cursorPos.x() += glyph.advance * toFontSize.x();
		bounds.x() = math::max(bounds.x(), cursorPos.x());
		bounds.y() = math::max(bounds.y(), cursorPos.y());
		return true;
	};

	uIndex idxSegment = 0;
	uIndex idxSegmentChar = 0;
	uIndex idxTotalChar = 0;
	bool bHasFinished = this->mUncommitted.segments.size() == 0;
	while (!bHasFinished)
	{
		Segment const& segment = this->segmentAt(idxSegment, idxSegmentChar, idxTotalChar);
		auto c = this->charAt(idxSegment, idxSegmentChar, idxTotalChar);
		if (!pushChar(c, segment)) break;
		bHasFinished = this->incrementChar(idxSegment, idxSegmentChar, idxTotalChar);
		++idxTotalChar;
	}
	return bounds;
}

Text::Segment const& Text::segmentAt(uIndex idxSegment, uIndex idxSegmentChar, uIndex idxTotalChar) const
{
	return this->mUncommitted.segments[idxSegment];
}

char Text::charAt(uIndex idxSegment, uIndex idxSegmentChar, uIndex idxTotalChar) const
{
	return this->mUncommitted.segments[idxSegment].content[idxSegmentChar];
}

bool Text::incrementChar(uIndex &idxSegment, uIndex &idxSegmentChar, uIndex idxTotalChar) const
{
	auto const& segment = this->mUncommitted.segments[idxSegment];
	if (++idxSegmentChar >= segment.content.length())
	{
		idxSegmentChar = 0;
		++idxSegment;
	}
	return /*bHasFinished*/ idxSegment >= this->mUncommitted.segments.size();
}

math::Vector2 Text::glyphToFontSize(
	graphics::Font::GlyphSprite const& glyph, f32 fontHeight
) const
{
	return math::Vector2({
		glyph.pointBasisRatio, (1.0f / glyph.pointBasisRatio)
	}) * fontHeight;
}

void Text::pushGlyph(
	math::Vector2 const& cursorPos,
	graphics::Font::GlyphSprite const& glyph,
	Segment const& segment
)
{
	if (glyph.size.x() <= 0 || glyph.size.y() <= 0)
	{
		return;
	}

	auto const widthEdge = this->widthEdge();
	f32 fontHeight = this->getFontHeight();
	auto toFontSize = this->glyphToFontSize(glyph, fontHeight);
	math::Vector2 screenBearing = glyph.bearing.toFloat() * toFontSize;
	math::Vector2 screenSize = glyph.size.toFloat() * toFontSize;

	auto pushVertex = [&](
		math::Vector2 const& sizeMult
	) -> ui16 {
		return this->pushVertex(Text::Vertex{
			(
				cursorPos + screenBearing + (screenSize * sizeMult)
			).createSubvector<4>() + widthEdge,
			glyph.atlasPos + (glyph.atlasSize * sizeMult),
			segment.color
		});
	};

	auto idxTL = pushVertex({ 0, 0 });
	auto idxTR = pushVertex({ 1, 0 });
	auto idxBR = pushVertex({ 1, 1 });
	auto idxBL = pushVertex({ 0, 1 });
	this->mIndices.push_back(idxTL);
	this->mIndices.push_back(idxTR);
	this->mIndices.push_back(idxBR);
	this->mIndices.push_back(idxBR);
	this->mIndices.push_back(idxBL);
	this->mIndices.push_back(idxTL);
}

void Text::record(graphics::Command *command)
{
	OPTICK_EVENT();
	if ((this->mOldVBuffer.isValid() || this->mOldIBuffer.isValid()) && this->mFramesSinceBufferRecreation++ >= 3)
	{
		this->mOldVBuffer.invalidate();
		this->mOldIBuffer.invalidate();
		this->mFramesSinceBufferRecreation = 0;
	}

	assert(this->mVertices.size() * sizeof(Vertex) <= this->mVertexBuffer.getSize());
	assert(this->mIndices.size() * sizeof(ui16) <= this->mIndexBuffer.getSize());
	if (this->mCommittedIndexCount == 0)
	{
		return;
	}
	auto pipeline = this->renderer()->textPipeline();
	command->bindPipeline(pipeline);
	command->bindDescriptorSets(pipeline, { &this->getFont()->descriptorSet() });
	command->bindVertexBuffers(0, { &this->mVertexBuffer });
	command->bindIndexBuffer(0, &this->mIndexBuffer, vk::IndexType::eUint16);
	command->draw(0, this->mCommittedIndexCount, 0, 0, 1);
}

