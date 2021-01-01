#include "ui/TextWidget.hpp"

#include "graphics/Command.hpp"
#include "graphics/FontAtlas.hpp"
#include "ui/WidgetRenderer.hpp"

using namespace ui;

Text::Text() : ui::Widget()
{
	this->mUncommitted.thickness = 0.5f;
	this->mUncommitted.edgeWidth = 0.1f;
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
Text& Text::setAnchor(math::Vector2 const& anchor) { Widget::setAnchor(anchor); return *this; }
Text& Text::setPivot(math::Vector2 const& pivot) { Widget::setPivot(pivot); return *this; }
Text& Text::setPosition(math::Vector2Int const& points) { Widget::setPosition(points); return *this; }
Text& Text::setSize(math::Vector2UInt const& points) { Widget::setSize(points); return *this; }
Text& Text::setFillWidth(bool bFill) { Widget::setFillWidth(bFill); return *this; }
Text& Text::setFillHeight(bool bFill) { Widget::setFillHeight(bFill); return *this; }
Text& Text::setZLayer(ui32 z) { Widget::setZLayer(z); return *this; }

Text& Text::setFont(std::string const& fontId)
{
	this->mUncommitted.fontId = fontId;
	return *this;
}

Text& Text::setFontSize(ui16 fontSize)
{
	this->mUncommitted.fontSize = fontSize;
	return *this;
}

Text& Text::setContent(std::string const& content)
{
	this->mUncommitted.content = content;
	return *this;
}

Text& Text::operator=(std::string const& content) { return setContent(content); }

Text& Text::setThickness(f32 characterWidth) { this->mUncommitted.thickness = characterWidth; return *this; }
Text& Text::setEdgeWidth(f32 charEdgeWidth) { this->mUncommitted.edgeWidth = charEdgeWidth; return *this; }

void Text::releaseGraphics()
{
	this->mVertexBuffer.invalidate();
	this->mIndexBuffer.invalidate();
}

Text& Text::commit(graphics::CommandPool* transientPool)
{
	// TODO: Check if there are actually any changes before updating buffers (this needs to include resolution)

	bool bRequiresNewBuffers = this->mUncommitted.content.length() != this->mCommitted.content.length();
	if (bRequiresNewBuffers)
	{
		// TODO: Can optimize by keeping the larger size so fewer buffer re-creations happen
		this->mVertexBuffer.invalidate();
		this->mIndexBuffer.invalidate();
		this->mVertices.clear();
		this->mIndices.clear();
		auto vertexCount = this->mUncommitted.content.length() * 4;
		auto indexCount = this->mUncommitted.content.length() * 6;
		this->mVertices.reserve(vertexCount);
		this->mIndices.reserve(vertexCount);
		this->mVertexBuffer
			.setSize(vertexCount * sizeof(Vertex))
			.create();
		this->mIndexBuffer
			.setSize(indexCount * sizeof(ui16))
			.create();
	}

	this->populateBufferData();

	// TODO: Can optimize by only writing changed regions (based on the diff between content and committed content, and if the font has changed)
	this->mVertexBuffer.writeBuffer(transientPool, 0, this->mVertices);
	this->mIndexBuffer.writeBuffer(transientPool, 0, this->mIndices);

	this->mCommitted = this->mUncommitted;
	return *this;
}

graphics::Font const* Text::getFont() const
{
	return &this->mpFontOwner.lock()->getFont(this->mUncommitted.fontId);
}

math::Vector2 Text::getSizeOnScreen() const
{
	auto const& content = this->mUncommitted.content;
	auto len = content.length();
	auto const& font = *this->getFont();
	f32 fontHeight = this->resolution().pointsToScreenSpace({ 0, this->mUncommitted.fontSize }).y();

	auto size = math::Vector2::ZERO;
	for (uIndex idxChar = 0; idxChar < len; ++idxChar)
	{
		auto const& glyph = font[content[idxChar]];
		auto toFontSize = math::Vector2({ glyph.pointBasisRatio, (1.0f / glyph.pointBasisRatio) }) * fontHeight;
		if (glyph.size.x() > 0 && glyph.size.y() > 0)
		{
			size.y() = math::max(size.y(), (glyph.size * toFontSize).y());
		}
		size.x() += glyph.advance * toFontSize.x();
	}

	return size;
}

void Text::populateBufferData()
{
	auto const& content = this->mUncommitted.content;
	auto len = content.length();
	graphics::Font const& font = *this->getFont();
	f32 fontHeight = this->resolution().pointsToScreenSpace({ 0, this->mUncommitted.fontSize }).y();	

	auto widthAndEdge = math::Vector2({ this->mUncommitted.thickness, this->mUncommitted.edgeWidth }).createSubvector<4>(2);
	auto const topLeft = this->getTopLeftPositionOnScreen();
	auto cursorPos = topLeft;

	struct ScreenGlyph
	{
		math::Vector2 screenBearing;
		math::Vector2 screenSize;
		math::Vector2 atlasPos;
		math::Vector2 atlasSize;
	};

	auto pushVertex = [&](
		math::Vector2 const& cursorPos, ScreenGlyph const& glyph,
		math::Vector2 const& sizeMult
	) -> ui16 {
		uIndex const idxVertex = this->mVertices.size();
		this->mVertices.push_back(Text::Vertex {
			(
				cursorPos + glyph.screenBearing + (glyph.screenSize * sizeMult)
			).createSubvector<4>() + widthAndEdge,
			glyph.atlasPos + (glyph.atlasSize * sizeMult)
		});
		return (ui16)idxVertex;
	};

	auto pushGlyph = [&](
		math::Vector2 const& cursorPos, ScreenGlyph const& glyph
	) {
		auto idxTL = pushVertex(cursorPos, glyph, { 0, 0 });
		auto idxTR = pushVertex(cursorPos, glyph, { 1, 0 });
		auto idxBR = pushVertex(cursorPos, glyph, { 1, 1 });
		auto idxBL = pushVertex(cursorPos, glyph, { 0, 1 });
		this->mIndices.push_back(idxTL);
		this->mIndices.push_back(idxTR);
		this->mIndices.push_back(idxBR);
		this->mIndices.push_back(idxBR);
		this->mIndices.push_back(idxBL);
		this->mIndices.push_back(idxTL);
	};

	for (uIndex idxChar = 0; idxChar < len; ++idxChar)
	{
		auto const& glyph = font[content[idxChar]];
		auto toFontSize = math::Vector2({ glyph.pointBasisRatio, (1.0f / glyph.pointBasisRatio) }) * fontHeight;
		if (glyph.size.x() > 0 && glyph.size.y() > 0)
		{
			pushGlyph(cursorPos, ScreenGlyph {
				glyph.bearing.toFloat() * toFontSize,
				glyph.size.toFloat() * toFontSize,
				glyph.atlasPos, glyph.atlasSize
			});
		}
		cursorPos.x() += glyph.advance * toFontSize.x();
	}
}

void Text::record(graphics::Command *command)
{
	OPTICK_EVENT();
	auto pipeline = this->renderer()->textPipeline();
	command->bindPipeline(pipeline);
	command->bindDescriptorSets(pipeline, { &this->getFont()->descriptorSet() });
	command->bindVertexBuffers(0, { &this->mVertexBuffer });
	command->bindIndexBuffer(0, &this->mIndexBuffer, vk::IndexType::eUint16);
	command->draw(0, (ui32)this->mIndices.size(), 0, 0, 1);
}

