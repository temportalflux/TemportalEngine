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
		.addAttribute({ 1, /*vec2*/ (ui32)vk::Format::eR32G32Sfloat, offsetof(Text::Vertex, texCoord) });
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

Text& Text::setMaxContentLength(ui32 length)
{
	this->lock();
	this->mUncommitted.maxLength = length;
	this->markDirty();
	this->unlock();
	return *this;
}

Text& Text::setContent(std::string const& content, bool expandMaxLength)
{
	this->lock();
	this->mUncommitted.content = content;
	if (expandMaxLength)
	{
		this->mUncommitted.maxLength = math::max(
			this->mUncommitted.maxLength, (ui32)content.length()
		);
	}
	this->markDirty();
	this->unlock();
	return *this;
}

std::string& Text::uncommittedContent() { return this->mUncommitted.content; }

Text& Text::operator=(std::string const& content) { return setContent(content); }

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
	this->mVertices.reserve(this->mUncommitted.content.length() * 4);
	this->mIndices.reserve(this->mUncommitted.content.length() * 6);

	if (this->desiredCharacterCount() != this->mBufferCharacterCount)
	{
		this->mBufferCharacterCount = this->desiredCharacterCount();
		this->mVertexBuffer.invalidate();
		this->mIndexBuffer.invalidate();
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
	return math::max(this->mUncommitted.maxLength, (ui32)this->contentLength());
}

graphics::Font const* Text::getFont() const
{
	return &this->mpFontOwner.lock()->getFont(this->mUncommitted.fontId);
}

math::Vector2 Text::getSizeOnScreen() const
{
	// for debugging
	auto const& content = this->mUncommitted.content;
	
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
		std::bind(&Text::pushGlyph, this, std::placeholders::_1, std::placeholders::_2)
	);
}

f32 Text::getFontHeight() const
{
	return this->toScreenHeight(this->mUncommitted.fontSize);
}

uSize Text::contentLength() const
{
	return this->mUncommitted.content.length();
}

char Text::charAt(uIndex i) const
{
	return this->mUncommitted.content[i];
}

math::Vector2 Text::writeGlyphs(
	math::Vector2 const& offset, math::Vector2 &cursorPos, math::Vector2 const& maxBounds,
	TDrawGlyph drawer
) const
{
	f32 fontHeight = this->getFontHeight();
	f32 lineHeight = fontHeight + this->toScreenHeight(5);

	auto const len = this->contentLength();
	graphics::Font const& font = *this->getFont();
	auto bounds = math::Vector2({ 0, lineHeight });
	for (uIndex idxChar = 0; idxChar < len; ++idxChar)
	{
		auto c = this->charAt(idxChar);
		if (c == '\n' || c == '\r')
		{
			cursorPos.y() += lineHeight;
			cursorPos.x() = 0;
			bounds.y() += lineHeight;
			continue;
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
			break;
		}
		if (drawer) drawer(offset + cursorPos, glyph);
		cursorPos.x() += glyph.advance * toFontSize.x();
		bounds.x() = math::max(bounds.x(), cursorPos.x());
		bounds.y() = math::max(bounds.y(), cursorPos.y());
	}
	return bounds;
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
	graphics::Font::GlyphSprite const& glyph
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
			glyph.atlasPos + (glyph.atlasSize * sizeMult)
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

