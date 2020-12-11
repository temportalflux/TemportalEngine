#include "render/ui/UIRenderer.hpp"

#include "Engine.hpp"
#include "logging/Logger.hpp"

#include "asset/Font.hpp"
#include "asset/PipelineAsset.hpp"
#include "asset/Shader.hpp"
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

UIRenderer& UIRenderer::setTextPipeline(std::shared_ptr<asset::Pipeline> asset)
{
	if (!this->mText.pipeline)
	{
		this->mText.pipeline = std::make_shared<graphics::Pipeline>();
	}

	this->mText.pipeline->addViewArea(asset->getViewport(), asset->getScissor());
	this->mText.pipeline->setBlendMode(asset->getBlendMode());
	this->mText.pipeline->setFrontFace(asset->getFrontFace());
	this->mText.pipeline->setTopology(asset->getTopology());

	// Perform a synchronous load on each shader to create the shader modules
	this->mText.pipeline->addShader(asset->getVertexShader().load(asset::EAssetSerialization::Binary)->makeModule());
	this->mText.pipeline->addShader(asset->getFragmentShader().load(asset::EAssetSerialization::Binary)->makeModule());

	{
		ui8 slot = 0;
		this->mText.pipeline->setBindings({
				graphics::AttributeBinding(graphics::AttributeBinding::Rate::eVertex)
				.setStructType<FontGlyphVertex>()
				.addAttribute({ 0, /*vec4*/ (ui32)vk::Format::eR32G32B32A32Sfloat, offsetof(FontGlyphVertex, position) })
				.addAttribute({ 1, /*vec2*/ (ui32)vk::Format::eR32G32Sfloat, offsetof(FontGlyphVertex, texCoord) })
		});
	}

	// Create descriptor groups for the pipeline
	for (auto const& assetDescGroup : asset->getDescriptorGroups())
	{
		auto const& descriptors = assetDescGroup.descriptors;
		auto descriptorGroup = graphics::DescriptorGroup();
		descriptorGroup.setBindingCount(descriptors.size());
		for (uIndex i = 0; i < descriptors.size(); ++i)
		{
			descriptorGroup.addBinding(descriptors[i].id, i, descriptors[i].type, descriptors[i].stage);
		}
		this->mText.descriptorGroups.push_back(std::move(descriptorGroup));
	}

	return *this;
}

UIRenderer& UIRenderer::addFont(std::string fontId, std::shared_ptr<asset::Font> asset)
{
	OPTICK_EVENT();
	RegisteredFont entry;
	entry.font.loadGlyphSets(asset->getFontSizes(), asset->glyphSets());

	for (auto const& face : entry.font.faces())
	{
		FontFaceImage faceData;
		faceData.idxDescriptor = this->mText.fontFaceCount;
		this->mText.fontFaceCount++;

		faceData.image
			.setFormat(vk::Format::eR8G8B8A8Srgb)
			.setSize(math::Vector3UInt(face.getAtlasSize()).z(1))
			.setTiling(vk::ImageTiling::eOptimal)
			.setUsage(vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled);
		
		entry.faces.insert(std::make_pair(face.getFontSize(), std::move(faceData)));
	}

	this->mText.fonts.insert(std::make_pair(fontId, std::move(entry)));

	return *this;
}

void UIRenderer::setDevice(std::weak_ptr<graphics::GraphicsDevice> device)
{
	OPTICK_EVENT();
	this->mpDevice = device;
	for (auto& fontEntry : this->mText.fonts)
	{
		for (auto& faceImageEntry : fontEntry.second.faces)
		{
			faceImageEntry.second.image.setDevice(device);
			faceImageEntry.second.image.create();
			faceImageEntry.second.view.setDevice(device);
		}
	}

	this->mText.sampler.setDevice(device);
	this->mText.sampler.create();
	
	this->mText.pipeline->setDevice(device);

	this->mText.vertexBuffer.setDevice(device);
	this->mText.indexBuffer.setDevice(device);
	this->mText.vertexBuffer.create();
	this->mText.indexBuffer.create();
}

void UIRenderer::setRenderPass(std::shared_ptr<graphics::RenderPass> renderPass)
{
	this->mText.pipeline->setRenderPass(renderPass);
}

void UIRenderer::initializeData(graphics::CommandPool* transientPool, graphics::DescriptorPool *descriptorPool)
{
	OPTICK_EVENT();
	// Write face image data to the memory buffer
	for (auto& fontEntry : this->mText.fonts)
	{
		for (auto& fontFace : fontEntry.second.font.faces())
		{
			auto& faceImage = fontEntry.second.faces[fontFace.getFontSize()];

			auto& data = fontFace.getPixelData();
			faceImage.image.transitionLayout(vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal, transientPool);
			faceImage.image.writeImage((void*)data.data(), data.size() * sizeof(ui8), transientPool);
			faceImage.image.transitionLayout(vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eShaderReadOnlyOptimal, transientPool);

			faceImage.view
				.setImage(&faceImage.image, vk::ImageAspectFlagBits::eColor)
				.create();
		}
	}
}

void UIRenderer::setFrameCount(uSize frameCount)
{
	// We don't actually need frames to set the amount of descriptors to be used.
	// Each face for each font needs its own descriptor because each have a different image.
	// They all share the same format though, which is why they are in the same group.
	this->mText.descriptorGroups[0].setAmount(this->mText.fontFaceCount);
}

void UIRenderer::createDescriptors(graphics::DescriptorPool *descriptorPool)
{
	OPTICK_EVENT();
	for (auto& descriptorGroup : this->mText.descriptorGroups)
	{
		descriptorGroup.create(this->mpDevice.lock(), descriptorPool);
	}
}

void UIRenderer::attachDescriptors(
	std::unordered_map<std::string, std::vector<graphics::Buffer*>> &mutableUniforms
)
{
	OPTICK_EVENT();
	for (auto& fontEntry : this->mText.fonts)
	{
		for (auto& faceImageEntry : fontEntry.second.faces)
		{
			this->mText.descriptorGroups[0].attachToBinding(
				"fontAtlas", /*idxArchetype=*/ 0, /*idxSet=*/ faceImageEntry.second.idxDescriptor,
				vk::ImageLayout::eShaderReadOnlyOptimal, &faceImageEntry.second.view, &this->mText.sampler
			);
		}
	}
}

void UIRenderer::writeDescriptors(std::shared_ptr<graphics::GraphicsDevice> device)
{
	OPTICK_EVENT();
	for (auto& descriptorGroup : this->mText.descriptorGroups)
	{
		descriptorGroup.writeAttachments(device);
	}
}

void UIRenderer::createPipeline(math::Vector2UInt const& resolution)
{
	OPTICK_EVENT();
	this->mScreenResolution = resolution;
	this->mText.pipeline
		->setDescriptors(&this->mText.descriptorGroups)
		.setResolution(resolution)
		.create();
	// TODO: Will need to rebuild all glyphs again because the font measurements need to be in screen space but are in the font-glyphs in terms of pixels
}

void UIRenderer::record(graphics::Command *command, uIndex idxFrame, TGetGlobalDescriptorSet getGlobalDescriptorSet)
{
	OPTICK_EVENT();

	std::optional<uIndex> idxPrevDescriptorSet = std::nullopt;
	for (auto const& committedString : this->mText.committedData.strings)
	{
		OPTICK_GPU_EVENT("DrawString");
		OPTICK_TAG("StringId", committedString.stringId.c_str());
		if (idxPrevDescriptorSet != committedString.idxDescriptor)
		{
			command->bindDescriptorSets(this->mText.pipeline, {
				this->mText.descriptorGroups[0].getDescriptorSet(committedString.idxDescriptor)
			});
			command->bindPipeline(this->mText.pipeline);
			idxPrevDescriptorSet = committedString.idxDescriptor;
		}

		command->bindVertexBuffers(0, { &this->mText.vertexBuffer });
		command->bindIndexBuffer(0, &this->mText.indexBuffer, vk::IndexType::eUint16);

		command->draw(
			committedString.idxStartIndex,
			committedString.indexCount,
			// the actual indicies of vertex data will be the indices in the buffer + their starting offset
			committedString.vertexPreCount,
			0, 1
		);
	}
}

uIndex UIRenderer::getTextDescriptorIdx(std::string const& fontId, ui8 fontSize) const
{
	auto findFont = this->mText.fonts.find(fontId);
	assert(findFont != this->mText.fonts.end());
	auto findFace = findFont->second.faces.find(fontSize);
	assert(findFace != findFont->second.faces.end());
	return findFace->second.idxDescriptor;
}

void UIRenderer::destroyRenderChain()
{
	this->mText.pipeline->invalidate();
	for (auto& descriptorGroup : this->mText.descriptorGroups)
	{
		descriptorGroup.invalidate();
	}
}

void UIRenderer::destroyRenderDevices()
{
	this->mText.sampler.destroy();
	this->mText.fonts.clear();
	this->mText.vertexBuffer.destroy();
	this->mText.indexBuffer.destroy();
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
	this->lock();
	this->mText.uncommittedData.bHasChanges = true;
	this->updateGlyphVertices(pStr, iterGlyphStr->second);
	this->unlock();
}

math::Vector2 UIRenderer::measure(UIString const* pStr) const
{
	auto fontIter = this->mText.fonts.find(pStr->fontId());
	assert(fontIter != this->mText.fonts.end());
	RegisteredFont const& fontEntry = fontIter->second;
	auto const& fontFaceAtlas = fontEntry.font.getFace(pStr->fontSize());
	auto const measurement = fontFaceAtlas.measure(pStr->content());
	return measurement.first.toFloat() / this->mScreenResolution.toFloat();
}

void UIRenderer::updateGlyphVertices(UIString const* updatedString, GlyphString &glyphStr) const
{
	auto resolution = this->mScreenResolution.toFloat();

	bool bRequiresFullReset = true;
		//updatedString->fontId() != glyphStr.prev.fontId
		//|| updatedString->fontSize() != glyphStr.prev.fontSize;
	
	auto fontIter = this->mText.fonts.find(updatedString->fontId());
	assert(fontIter != this->mText.fonts.end());
	RegisteredFont const& fontEntry = fontIter->second;
	auto const& fontFaceAtlas = fontEntry.font.getFace(updatedString->fontSize());
	
	auto const& content = updatedString->content();
	uSize strSize = content.length();
	if (bRequiresFullReset)
	{
		glyphStr.vertices.clear();
		glyphStr.vertices.reserve(strSize * 4);
		glyphStr.indices.clear();
		glyphStr.indices.reserve(strSize * 6);

		auto pushVertex = [&glyphStr, resolution](math::Vector2 const& pos, Font::GlyphData const& glyph, math::Vector2 const& sizeMult) -> ui16 {
			auto const idxVertex = glyphStr.vertices.size();
			FontGlyphVertex vertex;
			vertex.position = pos.createSubvector<4>(0) + (glyph.bearing + (glyph.size * sizeMult)).createSubvector<4>(2);
			vertex.texCoord = glyph.uvPos + (glyph.uvSize * sizeMult);
			glyphStr.vertices.push_back(vertex);
			return (ui16)idxVertex;
		};

		math::Vector2 glyphPos = updatedString->position();
		// Screen space is [-1,1] but positions are provided in [0,1] space
		glyphPos = glyphPos * 2.0f - 1.0f;

		auto const measurement = fontFaceAtlas.measure(content);
		//auto const sizeScreenSpace = measurement.first.toFloat() / resolution;
		// Shift the pos of the string down if characters have a up-offset from their center line
		glyphPos -= measurement.second.toFloat() / resolution;

		for (uIndex idxChar = 0; idxChar < strSize; ++idxChar)
		{
			auto glyph = fontFaceAtlas.getGlyph(content[idxChar]);
			assert(glyph.has_value());
			glyph->bearing /= resolution;
			glyph->size /= resolution;

			auto idxTL = pushVertex(glyphPos, *glyph, { 0, 0 });
			auto idxTR = pushVertex(glyphPos, *glyph, { 1, 0 });
			auto idxBR = pushVertex(glyphPos, *glyph, { 1, 1 });
			auto idxBL = pushVertex(glyphPos, *glyph, { 0, 1 });
			glyphStr.indices.push_back(idxTL);
			glyphStr.indices.push_back(idxTR);
			glyphStr.indices.push_back(idxBR);
			glyphStr.indices.push_back(idxBR);
			glyphStr.indices.push_back(idxBL);
			glyphStr.indices.push_back(idxTL);

			// Offset the position of the next glyph
			glyphPos.x() += glyph->advance / resolution.x();
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
		auto idxDescriptor = this->getTextDescriptorIdx(data.prev.fontId, data.prev.fontSize);
		auto committedString = GlyphStringIndices{ stringId, idxDescriptor };
		// Determines the order of the string ids according to their descriptor index
		committed.strings.insert(
			std::upper_bound(committed.strings.begin(), committed.strings.end(), committedString, [](
				GlyphStringIndices const& a, GlyphStringIndices const& b
			) {
				return a.idxDescriptor < b.idxDescriptor;
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
