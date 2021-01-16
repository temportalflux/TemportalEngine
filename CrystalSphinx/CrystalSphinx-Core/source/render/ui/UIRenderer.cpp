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

using namespace graphics;

static logging::Logger UIRenderLog = DeclareLog("UIRenderer", LOG_INFO);

UIRenderer::UIRenderer()
{
	this->textSampler()
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
}

UIRenderer::~UIRenderer()
{
	destroyRenderDevices();
}

UIRenderer& UIRenderer::setImagePipeline(asset::TypedAssetPath<asset::Pipeline> const& path)
{
	ui::WidgetRenderer::setImagePipeline(path);
	return *this;
}

UIRenderer& UIRenderer::setTextPipeline(asset::TypedAssetPath<asset::Pipeline> const& path)
{
	ui::WidgetRenderer::setTextPipeline(path);
	return *this;
}

UIRenderer& UIRenderer::addFont(std::string fontId, std::shared_ptr<asset::Font> asset)
{
	OPTICK_EVENT();
	auto font = graphics::Font();
	font.setSampler(&this->textSampler());
	auto glyphs = std::vector<asset::Font::Glyph>();
	asset->getSDF(font.atlasSize(), font.atlasPixels(), glyphs);
	
	for (auto const& glyph : glyphs)
	{
		font.addGlyph(glyph.asciiId, std::move(graphics::Font::GlyphSprite {
			glyph.atlasPos, glyph.atlasSize,
			glyph.pointBasis, glyph.size, glyph.bearing, glyph.advance
		}));
	}

	uIndex idx = this->mFonts.size();
	this->mFonts.push_back(std::move(font));
	this->mFontIds.insert(std::make_pair(fontId, idx));
	return *this;
}

void UIRenderer::setDevice(std::weak_ptr<graphics::GraphicsDevice> device)
{
	OPTICK_EVENT();
	ui::WidgetRenderer::setDevice(device);
	for (auto& font : this->mFonts)
	{
		font.setDevice(device);
	}
}

void UIRenderer::setRenderPass(std::shared_ptr<graphics::RenderPass> renderPass)
{
	this->imagePipeline()->setRenderPass(renderPass);
	this->textPipeline()->setRenderPass(renderPass);
}

void UIRenderer::initializeData(graphics::CommandPool* transientPool, graphics::DescriptorPool *descriptorPool)
{
	OPTICK_EVENT();
	for (auto& font : this->mFonts)
	{
		this->textDescriptorLayout().createSet(descriptorPool, font.descriptorSet());
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
	ui::WidgetRenderer::createPipeline(resolution);
}

void UIRenderer::record(graphics::Command *command, uIndex idxFrame, TGetGlobalDescriptorSet getGlobalDescriptorSet)
{
	ui::WidgetRenderer::record(command);
}

void UIRenderer::destroyRenderChain()
{
	this->imagePipeline()->invalidate();
	this->textPipeline()->invalidate();
}

void UIRenderer::destroyRenderDevices()
{
	this->imageSampler().destroy();
	this->imageDescriptorLayout().invalidate();

	this->textSampler().destroy();
	this->textDescriptorLayout().invalidate();

	this->mFontIds.clear();
	this->mFonts.clear();
}

graphics::Font const& UIRenderer::getFont(std::string const& fontId) const
{
	auto fontIter = this->mFontIds.find(fontId);
	assert(fontIter != this->mFontIds.end());
	return this->mFonts[fontIter->second];
}
