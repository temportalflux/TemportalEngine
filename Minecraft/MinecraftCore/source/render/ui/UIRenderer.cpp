#include "render/ui/UIRenderer.hpp"

#include "Engine.hpp"
#include "logging/Logger.hpp"

#include "asset/Font.hpp"
#include "asset/PipelineAsset.hpp"
#include "asset/Shader.hpp"
#include "graphics/assetHelpers.hpp"
#include "graphics/Command.hpp"
#include "graphics/Pipeline.hpp"
#include "graphics/VulkanApi.hpp"
#include "render/ui/UIString.hpp"

using namespace graphics;

static logging::Logger UIRenderLog = DeclareLog("UIRenderer");

UIRenderer::UIRenderer(
	uSize maximumDisplayedCharacters
)
{
	this->mText.fontFaceCount = 0;
	
	this->mText.sampler
		.setFilter(graphics::FilterMode::Enum::Linear, graphics::FilterMode::Enum::Linear)
		.setAddressMode({
			graphics::SamplerAddressMode::Enum::ClampToEdge,
			graphics::SamplerAddressMode::Enum::ClampToEdge,
			graphics::SamplerAddressMode::Enum::ClampToEdge
		})
		.setAnistropy(std::nullopt)
		.setBorderColor(graphics::BorderColor::Enum::BlackOpaqueInt)
		.setNormalizeCoordinates(true)
		.setCompare(std::nullopt)
		.setMipLOD(graphics::SamplerLODMode::Enum::Nearest, 0, { 0, 0 });
	this->imageSampler()
		.setFilter(graphics::FilterMode::Enum::Nearest, graphics::FilterMode::Enum::Nearest)
		.setAddressMode({
			graphics::SamplerAddressMode::Enum::ClampToEdge,
			graphics::SamplerAddressMode::Enum::ClampToEdge,
			graphics::SamplerAddressMode::Enum::ClampToEdge
		})
		.setAnistropy(16.0f)
		.setBorderColor(graphics::BorderColor::Enum::BlackOpaqueInt)
		.setNormalizeCoordinates(true)
		.setCompare(std::nullopt)
		.setMipLOD(graphics::SamplerLODMode::Enum::Linear, 0, { 0, 0 });

	uSize const vBufferSize = maximumDisplayedCharacters * /*vertices per character*/ 4 * /*size per vertex*/ sizeof(FontGlyphVertex);
	uSize const iBufferSize = maximumDisplayedCharacters * /*indices per character*/ 6 * /*size per vertex*/ sizeof(ui16);
	UIRenderLog.log(LOG_INFO, "Allocating UI text buffer for %i characters (%i bytes)", maximumDisplayedCharacters, (vBufferSize + iBufferSize));
	this->mText.vertexBuffer
		.setUsage(vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eVertexBuffer, MemoryUsage::eGPUOnly)
		.setSize(vBufferSize);
	this->mText.indexBuffer
		.setUsage(vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eIndexBuffer, MemoryUsage::eGPUOnly)
		.setSize(iBufferSize);
}

UIRenderer::~UIRenderer()
{
	destroyRenderDevices();
}

void UIRenderer::lock()
{
	this->mMutex.lock();
}

void UIRenderer::unlock()
{
	this->mMutex.unlock();
}

bool UIRenderer::hasChanges() const
{
	return this->mText.uncommittedData.bHasChanges;
}

UIRenderer& UIRenderer::setTextPipeline(asset::TypedAssetPath<asset::Pipeline> const& path)
{
	if (!this->mText.pipeline)
	{
		this->mText.pipeline = std::make_shared<graphics::Pipeline>();
	}

	this->mText.pipeline->setDepthEnabled(false, false);
	graphics::populatePipeline(path, this->mText.pipeline.get(), &this->mText.descriptorLayout);

	{
		ui8 slot = 0;
		this->mText.pipeline->setBindings({
				graphics::AttributeBinding(graphics::AttributeBinding::Rate::eVertex)
				.setStructType<FontGlyphVertex>()
				.addAttribute({ 0, /*vec4*/ (ui32)vk::Format::eR32G32B32A32Sfloat, offsetof(FontGlyphVertex, positionAndWidthEdge) })
				.addAttribute({ 1, /*vec2*/ (ui32)vk::Format::eR32G32Sfloat, offsetof(FontGlyphVertex, texCoord) })
		});
	}

	return *this;
}

UIRenderer& UIRenderer::addFont(std::string fontId, std::shared_ptr<asset::Font> asset)
{
	OPTICK_EVENT();
	auto font = graphics::Font();
	font.setSampler(&this->mText.sampler);
	auto glyphs = std::vector<asset::Font::Glyph>();
	asset->getSDF(font.atlasSize(), font.atlasPixels(), glyphs);
	
	for (auto const& glyph : glyphs)
	{
		font.addGlyph(glyph.asciiId, std::move(graphics::Font::GlyphSprite {
			glyph.atlasPos, glyph.atlasSize,
			glyph.pointBasis, glyph.size, glyph.bearing, glyph.advance
		}));
	}

	uIndex idx = this->mText.fonts.size();
	this->mText.fonts.push_back(std::move(font));
	this->mText.fontIds.insert(std::make_pair(fontId, idx));
	return *this;
}

UIRenderer& UIRenderer::setImagePipeline(asset::TypedAssetPath<asset::Pipeline> const& path)
{
	ui::WidgetRenderer::setImagePipeline(path);
	return *this;
}

void UIRenderer::setDevice(std::weak_ptr<graphics::GraphicsDevice> device)
{
	OPTICK_EVENT();
	this->mpDevice = device;
	this->mText.descriptorLayout.setDevice(device).create();
	for (auto& font : this->mText.fonts)
	{
		font.setDevice(device);
	}

	this->mText.sampler.setDevice(device);
	this->mText.sampler.create();
	
	this->mText.pipeline->setDevice(device);

	this->mText.vertexBuffer.setDevice(device);
	this->mText.indexBuffer.setDevice(device);
	this->mText.vertexBuffer.create();
	this->mText.indexBuffer.create();

	ui::WidgetRenderer::setDevice(device);
}

void UIRenderer::setRenderPass(std::shared_ptr<graphics::RenderPass> renderPass)
{
	this->mText.pipeline->setRenderPass(renderPass);
	this->imagePipeline()->setRenderPass(renderPass);
}

void UIRenderer::initializeData(graphics::CommandPool* transientPool, graphics::DescriptorPool *descriptorPool)
{
	OPTICK_EVENT();
	for (auto& font : this->mText.fonts)
	{
		this->mText.descriptorLayout.createSet(descriptorPool, font.descriptorSet());
		font.initializeImage(transientPool);
	}
	ui::WidgetRenderer::initializeData(transientPool, descriptorPool);
}

void UIRenderer::setFrameCount(uSize frameCount)
{
}

void UIRenderer::createDescriptors(graphics::DescriptorPool *descriptorPool)
{
}

void UIRenderer::attachDescriptors(
	std::unordered_map<std::string, std::vector<graphics::Buffer*>> &mutableUniforms
)
{
}

void UIRenderer::writeDescriptors(std::shared_ptr<graphics::GraphicsDevice> device)
{
}

void UIRenderer::createPipeline(math::Vector2UInt const& resolution)
{
	OPTICK_EVENT();
	this->mScreenResolution = resolution;
	this->mText.pipeline
		->setDescriptorLayout(this->mText.descriptorLayout, 1)
		.setResolution(resolution)
		.create();
	for (auto& [id, glyphString] : this->mText.uncommittedData.strings)
	{
		updateGlyphString(glyphString);
	}

	ui::WidgetRenderer::createPipeline(resolution);
}

void UIRenderer::record(graphics::Command *command, uIndex idxFrame, TGetGlobalDescriptorSet getGlobalDescriptorSet)
{
	OPTICK_EVENT();

	std::optional<uIndex> idxPrevFont = std::nullopt;
	for (auto const& committedString : this->mText.committedData.strings)
	{
		OPTICK_GPU_EVENT("DrawString");
		OPTICK_TAG("StringId", committedString.stringId.c_str());
		if (idxPrevFont != committedString.idxFont)
		{
			idxPrevFont = committedString.idxFont;
			command->bindDescriptorSets(this->mText.pipeline, {
				&this->mText.fonts[*idxPrevFont].descriptorSet()
			});
			command->bindPipeline(this->mText.pipeline);
			command->bindVertexBuffers(0, { &this->mText.vertexBuffer });
			command->bindIndexBuffer(0, &this->mText.indexBuffer, vk::IndexType::eUint16);
		}

		command->draw(
			committedString.idxStartIndex,
			committedString.indexCount,
			// the actual indices of vertex data will be the indices in the buffer + their starting offset
			committedString.vertexPreCount,
			0, 1
		);
	}

	ui::WidgetRenderer::record(command);
}

void UIRenderer::destroyRenderChain()
{
	this->mText.pipeline->invalidate();
	this->imagePipeline()->invalidate();
}

void UIRenderer::destroyRenderDevices()
{
	this->mText.sampler.destroy();
	this->imageSampler().destroy();
	this->mText.descriptorLayout.invalidate();
	this->mText.fonts.clear();
	this->mText.vertexBuffer.destroy();
	this->mText.indexBuffer.destroy();
	this->imageDescriptorLayout().invalidate();
}

void UIRenderer::addString(std::shared_ptr<UIString> pStr)
{
	auto& stringData = this->mText.uncommittedData;
	assert(stringData.stringIds.find(pStr->id()) == stringData.stringIds.end());
	this->lock();
	this->mText.uncommittedData.bHasChanges = true;
	stringData.stringIds.insert(pStr->id());
	stringData.strings.insert(std::make_pair(pStr->id(), GlyphString { pStr }));
	this->unlock();
}

void UIRenderer::removeString(UIString const* pStr)
{
	auto& stringData = this->mText.uncommittedData;
	assert(stringData.stringIds.find(pStr->id()) != stringData.stringIds.end());
	this->lock();
	this->mText.uncommittedData.bHasChanges = true;
	stringData.stringIds.erase(pStr->id());
	stringData.strings.erase(pStr->id());
	this->unlock();
}

void UIRenderer::updateString(UIString const* pStr)
{
	auto& stringData = this->mText.uncommittedData;
	auto iterGlyphStr = stringData.strings.find(pStr->id());
	assert(iterGlyphStr != stringData.strings.end());
	updateGlyphString(iterGlyphStr->second);
}

void UIRenderer::updateGlyphString(GlyphString &glyphStr)
{
	this->lock();
	this->mText.uncommittedData.bHasChanges = true;
	this->updateGlyphVertices(glyphStr.handle.get(), glyphStr);
	this->unlock();
}

void UIRenderer::updateGlyphVertices(UIString const* updatedString, GlyphString &glyphStr) const
{
	auto resolution = this->mScreenResolution.toFloat();
	if (resolution.y() <= 0) return;
	f32 aspectRatio = resolution.x() / resolution.y();

	auto widthAndEdge = math::Vector2({
		updatedString->thickness(), updatedString->edgeDistance()
	}).createSubvector<4>(2);

	bool bRequiresFullReset = true;
		//updatedString->fontId() != glyphStr.prev.fontId
		//|| updatedString->fontSize() != glyphStr.prev.fontSize;

	auto fontIter = this->mText.fontIds.find(updatedString->fontId());
	assert(fontIter != this->mText.fontIds.end());
	graphics::Font const& font = this->mText.fonts[fontIter->second];

	f32 fontHeight = updatedString->pixelHeight() / resolution.y();
	
	auto const& content = updatedString->content();
	uSize strSize = content.length();
	if (bRequiresFullReset)
	{
		glyphStr.vertices.clear();
		glyphStr.vertices.reserve(strSize * 4);
		glyphStr.indices.clear();
		glyphStr.indices.reserve(strSize * 6);

		// top left position of the string
		math::Vector2 cursorPos = updatedString->position();
		// Screen space is [-1,1] but positions are provided in [0,1] space
		cursorPos *= 2.0f;
		cursorPos -= 1.0f;

		/**
		 * Measurement of the string in ratio of points (1/72 inches) to basis
		 */
		auto const measurement = font.measure(content);
		// The amount of screen space the bearing inhabits is the change of base from pixels (relative to its total height) to the desired height of the string
		f32 verticalBearing = measurement.z() * fontHeight;
		// shift the cursor down by vertical distance from the center line of the string to the top
		cursorPos.y() += verticalBearing;

		struct ScreenGlyph
		{
			math::Vector2 screenBearing;
			math::Vector2 screenSize;
			math::Vector2 atlasPos;
			math::Vector2 atlasSize;
		};

		auto pushVertex = [&glyphStr, widthAndEdge](
			math::Vector2 const& cursorPos, ScreenGlyph const& glyph,
			math::Vector2 const& sizeMult
		) -> ui16 {
			uIndex const idxVertex = glyphStr.vertices.size();
			glyphStr.vertices.push_back(FontGlyphVertex {
				(
					cursorPos + glyph.screenBearing + (glyph.screenSize * sizeMult)
				).createSubvector<4>() + widthAndEdge,
				glyph.atlasPos + (glyph.atlasSize * sizeMult)
			});
			return (ui16)idxVertex;
		};

		auto pushGlyph = [&glyphStr, pushVertex](
			math::Vector2 const& cursorPos, ScreenGlyph const& glyph
		) {
			auto idxTL = pushVertex(cursorPos, glyph, { 0, 0 });
			auto idxTR = pushVertex(cursorPos, glyph, { 1, 0 });
			auto idxBR = pushVertex(cursorPos, glyph, { 1, 1 });
			auto idxBL = pushVertex(cursorPos, glyph, { 0, 1 });
			glyphStr.indices.push_back(idxTL);
			glyphStr.indices.push_back(idxTR);
			glyphStr.indices.push_back(idxBR);
			glyphStr.indices.push_back(idxBR);
			glyphStr.indices.push_back(idxBL);
			glyphStr.indices.push_back(idxTL);
		};
		
		for (uIndex idxChar = 0; idxChar < strSize; ++idxChar)
		{
			auto const& glyph = font[content[idxChar]];
			auto toFontSize = math::Vector2({
				glyph.pointBasisRatio * fontHeight,
				(1.0f / glyph.pointBasisRatio) * fontHeight
			});
			if (glyph.size.x() > 0 && glyph.size.y() > 0)
			{
				pushGlyph(cursorPos, ScreenGlyph{
					glyph.bearing.toFloat() * toFontSize,
					glyph.size.toFloat() * toFontSize,
					glyph.atlasPos, glyph.atlasSize
				});
			}
			cursorPos.x() += glyph.advance * toFontSize.x();
		}
	}
	// Can update positions, change elements, or add/remove elements incrementally without necessarily updating EVERYTHING
	else
	{

	}

	glyphStr.prev.position = updatedString->position();
	glyphStr.prev.content = updatedString->content();
	glyphStr.prev.fontId = updatedString->fontId();
	glyphStr.prev.fontSize = updatedString->fontSize();
}

void UIRenderer::commitToBuffer(graphics::CommandPool* transientPool)
{
	OPTICK_EVENT();
	auto& uncommitted = this->mText.uncommittedData;
	auto& committed = this->mText.committedData;

	committed.strings.clear();
	committed.strings.reserve(uncommitted.stringIds.size());
	
	for (auto const& stringId : uncommitted.stringIds)
	{
		auto& data = uncommitted.strings.at(stringId);
		auto idxFont = this->mText.fontIds.find(data.prev.fontId);
		auto committedString = GlyphStringIndices{ stringId, idxFont->second };
		// Determines the order of the string ids according to their descriptor index
		committed.strings.insert(
			std::upper_bound(committed.strings.begin(), committed.strings.end(), committedString, [](
				GlyphStringIndices const& a, GlyphStringIndices const& b
			) {
				return a.idxFont < b.idxFont;
			}),
			committedString
		);
	}

	auto vertices = std::vector<FontGlyphVertex>();
	auto indices = std::vector<ui16>();
	for (auto& committedString : committed.strings)
	{
		auto& data = uncommitted.strings.at(committedString.stringId);
		committedString.vertexPreCount = (ui32)vertices.size();
		committedString.idxStartIndex = (ui32)indices.size();
		committedString.indexCount = (ui32)data.indices.size();
		for (auto const& vertex : data.vertices) vertices.push_back(vertex);
		for (auto const& index : data.indices) indices.push_back(index);
	}

	this->mText.vertexBuffer.writeBuffer(transientPool, 0, vertices);
	this->mText.indexBuffer.writeBuffer(transientPool, 0, indices);

	this->mText.uncommittedData.bHasChanges = false;
}
