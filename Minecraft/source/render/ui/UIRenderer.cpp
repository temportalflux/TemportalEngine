#include "render/ui/UIRenderer.hpp"

#include "Engine.hpp"
#include "logging/Logger.hpp"

#include "asset/Font.hpp"
#include "asset/PipelineAsset.hpp"
#include "asset/Shader.hpp"
#include "graphics/Memory.hpp"
#include "graphics/Pipeline.hpp"
#include "graphics/VulkanApi.hpp"

using namespace graphics;

static logging::Logger UIRenderLog = DeclareLog("UIRenderer");

UIRenderer::UIRenderer(
	std::weak_ptr<graphics::DescriptorPool> pDescriptorPool,
	uSize maximumDisplayedCharacters
)
{
	this->mpDescriptorPool = pDescriptorPool;

	this->mText.fontFaceCount = 0;
	this->mText.memoryFontImages = std::make_shared<Memory>();
	this->mText.memoryFontImages->setFlags(vk::MemoryPropertyFlagBits::eDeviceLocal);

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

	this->mText.memoryTextBuffers = std::make_shared<graphics::Memory>();
	this->mText.memoryTextBuffers->setFlags(vk::MemoryPropertyFlagBits::eDeviceLocal);

	uSize const vBufferSize = maximumDisplayedCharacters * /*vertices per character*/ 4 * /*size per vertex*/ sizeof(FontGlyphVertex);
	uSize const iBufferSize = maximumDisplayedCharacters * /*indices per character*/ 6 * /*size per vertex*/ sizeof(ui16);
	UIRenderLog.log(LOG_INFO, "Allocating UI text buffer for %i characters (%i bytes)", maximumDisplayedCharacters, (vBufferSize + iBufferSize));
	this->mText.vertexBuffer.setUsage(vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eVertexBuffer).setSize(vBufferSize);
	this->mText.indexBuffer.setUsage(vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eIndexBuffer).setSize(iBufferSize);
}

UIRenderer::~UIRenderer()
{
	destroyRenderDevices();
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
	this->mText.memoryFontImages->setDevice(device);
	for (auto& fontEntry : this->mText.fonts)
	{
		for (auto& faceImageEntry : fontEntry.second.faces)
		{
			faceImageEntry.second.image.setDevice(device);
			faceImageEntry.second.image.create();
			// Configure the image so the memory object knows how much to allocate on the GPU
			// Has to be done after the internal Vulkan image is created above
			faceImageEntry.second.image.configureSlot(this->mText.memoryFontImages);

			faceImageEntry.second.view.setDevice(device);
		}
	}
	// Allocate the memory buffer for the face images
	this->mText.memoryFontImages->create();

	this->mText.sampler.setDevice(device);
	this->mText.sampler.create();
	
	this->mText.pipeline->setDevice(device);

	this->mText.memoryTextBuffers->setDevice(device);
	this->mText.vertexBuffer.setDevice(device);
	this->mText.indexBuffer.setDevice(device);
	this->mText.vertexBuffer.create();
	this->mText.vertexBuffer.configureSlot(this->mText.memoryTextBuffers);
	this->mText.indexBuffer.create();
	this->mText.indexBuffer.configureSlot(this->mText.memoryTextBuffers);
	this->mText.memoryTextBuffers->create();
	this->mText.vertexBuffer.bindMemory();
	this->mText.indexBuffer.bindMemory();
}

void UIRenderer::setRenderPass(std::shared_ptr<graphics::RenderPass> renderPass)
{
	this->mText.pipeline->setRenderPass(renderPass);
}

void UIRenderer::initializeData(graphics::CommandPool* transientPool)
{
	OPTICK_EVENT();
	// Write face image data to the memory buffer
	for (auto& fontEntry : this->mText.fonts)
	{
		for (auto& fontFace : fontEntry.second.font.faces())
		{
			auto& faceImage = fontEntry.second.faces[fontFace.getFontSize()];

			auto& data = fontFace.getPixelData();
			faceImage.image.bindMemory();
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
	// We dont actually need frames to set the amount of descriptors to be used.
	// Each face for each font needs its own descriptor because each have a different image.
	// They all share the same format though, which is why they are in the same group.
	this->mText.descriptorGroups[0].setAmount(this->mText.fontFaceCount);
}

void UIRenderer::createDescriptors(std::shared_ptr<graphics::GraphicsDevice> device)
{
	OPTICK_EVENT();
	for (auto& descriptorGroup : this->mText.descriptorGroups)
	{
		descriptorGroup.create(device, this->mpDescriptorPool.lock().get());
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
	this->mText.pipeline
		->setDescriptors(&this->mText.descriptorGroups)
		.setResolution(resolution)
		.create();
}

void UIRenderer::record(graphics::Command *command, uIndex idxFrame)
{
	//auto& descriptorSet = graphics::extract<vk::DescriptorSet>(this->getTextDescriptor("", 0));
}

void* UIRenderer::getTextDescriptor(std::string const& fontId, ui8 fontSize) const
{
	auto findFont = this->mText.fonts.find(fontId);
	assert(findFont != this->mText.fonts.end());
	auto findFace = findFont->second.faces.find(fontSize);
	assert(findFace != findFont->second.faces.end());
	return (void*)(&this->mText.descriptorGroups[0].getDescriptorSet(findFace->second.idxDescriptor));
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
	this->mText.memoryFontImages.reset();

	this->mText.vertexBuffer.destroy();
	this->mText.indexBuffer.destroy();
	this->mText.memoryTextBuffers.reset();
}
