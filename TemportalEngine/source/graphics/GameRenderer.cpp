#include "graphics/GameRenderer.hpp"

#include "IRender.hpp"
#include "asset/Font.hpp"
#include "asset/PipelineAsset.hpp"
#include "asset/RenderPassAsset.hpp"
#include "asset/Texture.hpp"
#include "asset/TextureSampler.hpp"
#include "graphics/StringRenderer.hpp"
#include "graphics/Uniform.hpp"
#include "graphics/VulkanApi.hpp"
#include "graphics/ShaderModule.hpp"

using namespace graphics;

GameRenderer::GameRenderer()
	: VulkanRenderer()
{
	this->mDescriptorPool.setPoolSize(64, {
		{ vk::DescriptorType::eUniformBuffer, 64 },
		{ vk::DescriptorType::eCombinedImageSampler, 64 },
	});

	this->mVertexBufferUI
		.setUsage(vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eVertexBuffer);
	this->mIndexBufferUI
		.setUsage(vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eIndexBuffer);
	
	// TODO: Use a `MemoryChunk` instead of global memory
	this->mpStringRenderer = std::make_shared<StringRenderer>();
	this->mpStringRenderer->initialize();
}

void GameRenderer::invalidate()
{
	this->mTextureDescriptorPairs.clear();
	this->mTextureViews.clear();
	this->mTextureImages.clear();
	this->mTextureSamplers.clear();

	this->mpMemoryImages.reset();

	this->mpStringRenderer.reset();
	this->mpMemoryFontImages.reset();

	this->destroyRenderChain();
	this->mUniformStaticBuffersPerFrame.clear();
	this->mpMemoryUniformBuffers->destroy();
	this->mPipeline.destroyShaders();
	this->mPipelineUI.destroyShaders();

	this->mVertexBufferUI.destroy();
	this->mIndexBufferUI.destroy();
	this->mpMemoryUIBuffers.reset();

	this->mpMemoryUniformBuffers.reset();

	VulkanRenderer::invalidate();
}

void GameRenderer::initializeDevices()
{
	VulkanRenderer::initializeDevices();

	this->initializeTransientCommandPool();

	// TODO: Use a `MemoryChunk` instead of global memory
	this->mpMemoryUniformBuffers = std::make_shared<graphics::Memory>();
	this->mpMemoryUniformBuffers->setDevice(this->mpGraphicsDevice);
	// Host Coherent means this entire buffer will be automatically flushed per write.
	// This can be optimized later by only flushing the portion of the buffer which actually changed.
	this->mpMemoryUniformBuffers->setFlags(vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);

	this->mpMemoryImages = std::make_shared<Memory>();
	this->mpMemoryImages->setDevice(this->mpGraphicsDevice);
	this->mpMemoryImages->setFlags(vk::MemoryPropertyFlagBits::eDeviceLocal);

	this->mpMemoryUIBuffers = std::make_shared<graphics::Memory>();
	this->mpMemoryUIBuffers->setDevice(this->mpGraphicsDevice);
	this->mpMemoryUIBuffers->setFlags(vk::MemoryPropertyFlagBits::eDeviceLocal);
}

std::shared_ptr<GraphicsDevice> GameRenderer::getDevice()
{
	return this->mpGraphicsDevice;
}

CommandPool& GameRenderer::getTransientPool()
{
	return this->mCommandPoolTransient;
}

void GameRenderer::addRender(IRender *render)
{
	this->mpRenders.push_back(render);
}

void GameRenderer::setStaticUniform(std::shared_ptr<Uniform> uniform)
{
	this->mpUniformStatic = uniform;
}

std::shared_ptr<StringRenderer> GameRenderer::stringRenderer()
{
	return this->mpStringRenderer;
}

void GameRenderer::initializeTransientCommandPool()
{
	this->mCommandPoolTransient.setDevice(this->mpGraphicsDevice);
	this->mCommandPoolTransient
		.setFlags(vk::CommandPoolCreateFlagBits::eTransient)
		.setQueueFamily(
			graphics::QueueFamily::Enum::eGraphics,
			this->mpGraphicsDevice->queryQueueFamilyGroup()
		)
		.create();
}

void GameRenderer::setBindings(std::vector<AttributeBinding> bindings)
{
	this->mPipeline.setBindings(bindings);
}

void GameRenderer::addShader(std::shared_ptr<ShaderModule> shader)
{
	this->mPipeline.addShader(shader);
}

void GameRenderer::setUIShaderBindings(std::shared_ptr<ShaderModule> shaderVert, std::shared_ptr<ShaderModule> shaderFrag, std::vector<AttributeBinding> bindings)
{
	this->mPipelineUI.addShader(shaderVert).addShader(shaderFrag).setBindings(bindings);
}

void GameRenderer::createRenderPass(std::shared_ptr<asset::RenderPass> asset)
{
	// TODO: Don't use default make_shared
	// TODO: Store by assetPath as key in a registry so the object doesnt go out of scope
	auto renderPass = std::make_shared<graphics::RenderPass>();
	renderPass->setDevice(this->mpGraphicsDevice);

	auto& colorAttachment = renderPass->addAttachment(
		RenderPassAttachment()
		//.setFormat(this->mSwapChain.getFormat())
		.setSamples(vk::SampleCountFlagBits::e1)
		.setGeneralOperations(vk::AttachmentLoadOp::eClear, vk::AttachmentStoreOp::eStore)
		.setStencilOperations(vk::AttachmentLoadOp::eDontCare, vk::AttachmentStoreOp::eDontCare)
		// Constant: Cannot put in asset
		.setLayouts(vk::ImageLayout::eUndefined, vk::ImageLayout::ePresentSrcKHR)
	);
	auto& depthAttachment = renderPass->addAttachment(
		RenderPassAttachment()
		//.setFormat((ui32)this->mDepthImage.getFormat())
		.setSamples(vk::SampleCountFlagBits::e1)
		.setGeneralOperations(vk::AttachmentLoadOp::eClear, vk::AttachmentStoreOp::eDontCare)
		.setStencilOperations(vk::AttachmentLoadOp::eDontCare, vk::AttachmentStoreOp::eDontCare)
		// Constant: Cannot put in asset
		.setLayouts(vk::ImageLayout::eUndefined, vk::ImageLayout::eDepthStencilAttachmentOptimal)
	);

	auto& onlyPhase = renderPass->addPhase(
		RenderPassPhase()
		.addColorAttachment(colorAttachment)
		.setDepthAttachment(depthAttachment)
	);

	renderPass->addDependency(
		{ std::nullopt, vk::PipelineStageFlagBits::eColorAttachmentOutput },
		{ onlyPhase, vk::PipelineStageFlagBits::eColorAttachmentOutput, vk::AccessFlagBits::eColorAttachmentWrite }
	);
	
	// TODO: Call during createRenderChain
	//renderPass->create();
}

uIndex GameRenderer::createTextureSampler(std::shared_ptr<asset::TextureSampler> sampler)
{
	auto addressModes = sampler->getAddressModes();
	auto compareOp = sampler->getCompareOperation();
	uIndex idx = this->mTextureSamplers.size();
	this->mTextureSamplers.push_back(graphics::ImageSampler());
	this->mTextureSamplers[idx].setDevice(this->mpGraphicsDevice);
	this->mTextureSamplers[idx]
		.setFilter(
			(vk::Filter)sampler->getFilterModeMagnified(),
			(vk::Filter)sampler->getFilterModeMinified()
		)
		.setAddressMode({
			(vk::SamplerAddressMode)addressModes[0],
			(vk::SamplerAddressMode)addressModes[1],
			(vk::SamplerAddressMode)addressModes[2]
		})
		.setAnistropy(sampler->getAnisotropy())
		.setBorderColor((vk::BorderColor)sampler->getBorderColor())
		.setNormalizeCoordinates(sampler->areCoordinatesNormalized())
		.setCompare(compareOp ? std::make_optional((vk::CompareOp)(*compareOp)) : std::nullopt)
		.setMipLOD(
			(vk::SamplerMipmapMode)sampler->getLodMode(),
			sampler->getLodBias(), sampler->getLodRange()
		);
	this->mTextureSamplers[idx].create();
	return idx;
}

uIndex GameRenderer::createTextureAssetImage(std::shared_ptr<asset::Texture> texture, uIndex idxSampler)
{
	auto idxImage = this->mTextureImages.size();
	this->mTextureImages.push_back(graphics::Image());
	this->mTextureImages[idxImage].setDevice(this->mpGraphicsDevice);
	this->mTextureImages[idxImage]
		.setFormat(vk::Format::eR8G8B8A8Srgb)
		.setSize(math::Vector3UInt(texture->getSourceSize()).z(1))
		.setTiling(vk::ImageTiling::eOptimal)
		.setUsage(vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled);
	this->mTextureImages[idxImage].create();
	this->mTextureImages[idxImage].configureSlot(this->mpMemoryImages);

	uIndex idx = this->mTextureViews.size();
	assert(idx == idxImage);

	this->mTextureViews.push_back(graphics::ImageView());
	this->mTextureViews[idx].setDevice(this->mpGraphicsDevice);

	this->mTextureDescriptorPairs.push_back(std::make_pair(idx, idxSampler));

	return idxImage;
}

void GameRenderer::allocateTextureMemory()
{
	this->mpMemoryImages->create();
}

void GameRenderer::writeTextureData(uIndex idxImage, std::shared_ptr<asset::Texture> texture)
{
	std::vector<ui8> data = texture->getSourceBinary();
	auto dataMemSize = texture->getSourceMemorySize();

	this->mTextureImages[idxImage].bindMemory();

	this->mTextureImages[idxImage].transitionLayout(vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal, &this->mCommandPoolTransient);
	this->mTextureImages[idxImage].writeImage((void*)data.data(), dataMemSize, &this->mCommandPoolTransient);
	this->mTextureImages[idxImage].transitionLayout(vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eShaderReadOnlyOptimal, &this->mCommandPoolTransient);

	this->mTextureViews[idxImage]
		.setImage(&this->mTextureImages[idxImage], vk::ImageAspectFlagBits::eColor)
		.create();
}

std::shared_ptr<StringRenderer> GameRenderer::setFont(std::shared_ptr<asset::Font> font)
{
	this->mpMemoryFontImages = std::make_shared<Memory>();
	this->mpMemoryFontImages->setDevice(this->mpGraphicsDevice);
	this->mpMemoryFontImages->setFlags(vk::MemoryPropertyFlagBits::eDeviceLocal);

	this->stringRenderer()->setFont(font->getFontSizes(), font->glyphSets());

	// Create the device objects for all the font images (samplers, images, and views)
	for (auto& face : this->stringRenderer()->getFont().faces())
	{
		face.sampler().setDevice(this->mpGraphicsDevice);
		face.sampler()
			.setFilter(vk::Filter::eLinear, vk::Filter::eLinear)
			.setAddressMode({ vk::SamplerAddressMode::eClampToEdge, vk::SamplerAddressMode::eClampToEdge, vk::SamplerAddressMode::eClampToEdge })
			.setAnistropy(std::nullopt)
			.setBorderColor(vk::BorderColor::eIntOpaqueBlack)
			.setNormalizeCoordinates(true)
			.setCompare(std::nullopt)
			.setMipLOD(vk::SamplerMipmapMode::eNearest, 0, { 0, 0 })
			.create();
		
		auto& image = face.image();
		image.setDevice(this->mpGraphicsDevice);
		image
			.setFormat(vk::Format::eR8G8B8A8Srgb)
			.setSize(math::Vector3UInt(face.getAtlasSize()).z(1))
			.setTiling(vk::ImageTiling::eOptimal)
			.setUsage(vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled);
		image.create();

		// Configure the image so the memory object knows how much to allocate on the GPU
		image.configureSlot(this->mpMemoryFontImages);
	}

	// Allocate the memory buffer for the face images
	this->mpMemoryFontImages->create();

	// Write face image data to the memory buffer
	for (auto& face : this->stringRenderer()->getFont().faces())
	{
		auto& data = face.getPixelData();
		face.image().bindMemory();
		face.image().transitionLayout(vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal, &this->mCommandPoolTransient);
		face.image().writeImage((void*)data.data(), data.size() * sizeof(ui8), &this->mCommandPoolTransient);
		face.image().transitionLayout(vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eShaderReadOnlyOptimal, &this->mCommandPoolTransient);

		face.view().setDevice(this->mpGraphicsDevice);
		face.view()
			.setImage(&face.image(), vk::ImageAspectFlagBits::eColor)
			.create();
	}

	return this->stringRenderer();
}

void GameRenderer::prepareUIBuffers(ui64 const maxCharCount)
{
	this->mVertexBufferUI.setDevice(this->mpGraphicsDevice);
	this->mVertexBufferUI
		.setSize(maxCharCount * /*verticies per character*/ 4 * sizeof(Font::UIVertex))
		.create();

	this->mIndexBufferUI.setDevice(this->mpGraphicsDevice);
	this->mIndexBufferUI
		.setSize(maxCharCount * /*indicies per character*/ 6 * sizeof(ui16))
		.create();

	this->mVertexBufferUI.configureSlot(this->mpMemoryUIBuffers);
	this->mIndexBufferUI.configureSlot(this->mpMemoryUIBuffers);
	
	this->mpMemoryUIBuffers->create();

	this->mVertexBufferUI.bindMemory();
	this->mIndexBufferUI.bindMemory();
}

void GameRenderer::createRenderChain()
{
	if (!this->mCommandPoolTransient.isValid())
	{
		this->initializeTransientCommandPool();
	}

	this->createSwapChain();

	this->stringRenderer()->setResolution(this->mSwapChain.getResolution());
	this->mIndexCountUI = this->stringRenderer()->writeBuffers(&this->mCommandPoolTransient, &this->mVertexBufferUI, &this->mIndexBufferUI);

	this->createFrameImageViews();
	this->createDepthResources(this->mSwapChain.getResolution());
	this->createRenderPass();

	this->createUniformBuffers();
	this->mDescriptorPool.setDevice(this->mpGraphicsDevice);
	this->mDescriptorPool.setAllocationMultiplier((ui32)this->mFrameImageViews.size()).create();
	this->createDescriptors();
	this->createCommandObjects();

	this->createFrames(this->mFrameImageViews.size());
}

void GameRenderer::destroyRenderChain()
{
	this->destroyFrames();

	this->destroyCommandObjects();
	this->mDescriptorGroupUI.invalidate();
	this->mDescriptorGroup.invalidate();
	this->mDescriptorPool.invalidate();

	this->destroyRenderPass();
	this->destroyDepthResources();
	this->destroyFrameImageViews();
	this->destroySwapChain();

	this->mCommandPoolTransient.destroy();
}

void GameRenderer::createRenderPass()
{
	this->mRenderPass.setDevice(this->mpGraphicsDevice);

	auto& colorAttachment = this->mRenderPass.addAttachment(
		RenderPassAttachment()
		.setFormat(this->mSwapChain.getFormat())
		.setSamples(vk::SampleCountFlagBits::e1)
		.setGeneralOperations(vk::AttachmentLoadOp::eClear, vk::AttachmentStoreOp::eStore)
		.setStencilOperations(vk::AttachmentLoadOp::eDontCare, vk::AttachmentStoreOp::eDontCare)
		.setLayouts(vk::ImageLayout::eUndefined, vk::ImageLayout::ePresentSrcKHR)
	);
	auto& depthAttachment = this->mRenderPass.addAttachment(
		RenderPassAttachment()
		.setFormat((ui32)this->mDepthImage.getFormat())
		.setSamples(vk::SampleCountFlagBits::e1)
		.setGeneralOperations(vk::AttachmentLoadOp::eClear, vk::AttachmentStoreOp::eDontCare)
		.setStencilOperations(vk::AttachmentLoadOp::eDontCare, vk::AttachmentStoreOp::eDontCare)
		.setLayouts(vk::ImageLayout::eUndefined, vk::ImageLayout::eDepthStencilAttachmentOptimal)
	);

	auto& onlyPhase = this->mRenderPass.addPhase(
		RenderPassPhase()
		.addColorAttachment(colorAttachment)
		.setDepthAttachment(depthAttachment)
	);

	this->mRenderPass.addDependency(
		{ std::nullopt, vk::PipelineStageFlagBits::eColorAttachmentOutput },
		{ onlyPhase, vk::PipelineStageFlagBits::eColorAttachmentOutput, vk::AccessFlagBits::eColorAttachmentWrite }
	);

	this->mRenderPass.create();
}

RenderPass* GameRenderer::getRenderPass()
{
	return &this->mRenderPass;
}

void GameRenderer::destroyRenderPass()
{
	this->mRenderPass.destroy();
}

void GameRenderer::createUniformBuffers()
{
	if (this->mpMemoryUniformBuffers->isValid()) { return; }
	auto frameCount = this->mFrameImageViews.size();
	this->mUniformStaticBuffersPerFrame.resize(frameCount);
	for (ui32 idxFrame = 0; idxFrame < frameCount; ++idxFrame)
	{
		auto& buffer = this->mUniformStaticBuffersPerFrame[idxFrame];
		buffer.setDevice(this->mpGraphicsDevice);
		buffer
			.setUsage(vk::BufferUsageFlagBits::eUniformBuffer)
			.setSize(this->mpUniformStatic->size());
		buffer.create();
		buffer.configureSlot(this->mpMemoryUniformBuffers);
	}
	this->mpMemoryUniformBuffers->setDevice(this->mpGraphicsDevice);
	this->mpMemoryUniformBuffers->create();
	for (ui32 idxFrame = 0; idxFrame < frameCount; ++idxFrame)
	{
		auto& buffer = this->mUniformStaticBuffersPerFrame[idxFrame];
		buffer.bindMemory();
	}
}

void GameRenderer::createDepthResources(math::Vector2UInt const &resolution)
{

	auto supportedFormat = this->mpGraphicsDevice->pickFirstSupportedFormat(
		{ vk::Format::eD32Sfloat, vk::Format::eD32SfloatS8Uint, vk::Format::eD24UnormS8Uint },
		vk::ImageTiling::eOptimal, vk::FormatFeatureFlagBits::eDepthStencilAttachment
	);
	if (!supportedFormat) throw std::runtime_error("failed to find supported depth buffer format");

	this->mpMemoryDepthImage = std::make_shared<Memory>();
	this->mpMemoryDepthImage->setDevice(this->mpGraphicsDevice);
	this->mpMemoryDepthImage->setFlags(vk::MemoryPropertyFlagBits::eDeviceLocal);

	this->mDepthImage.setDevice(this->mpGraphicsDevice);
	this->mDepthImage
		.setFormat(*supportedFormat)
		.setSize({ resolution.x(), resolution.y(), 1 })
		.setTiling(vk::ImageTiling::eOptimal)
		.setUsage(vk::ImageUsageFlagBits::eDepthStencilAttachment);
	this->mDepthImage.create();

	this->mDepthImage.configureSlot(this->mpMemoryDepthImage);
	this->mpMemoryDepthImage->create();
	this->mDepthImage.bindMemory();
	this->mDepthImage.transitionLayout(vk::ImageLayout::eUndefined, vk::ImageLayout::eDepthStencilAttachmentOptimal, &this->mCommandPoolTransient);

	this->mDepthView.setDevice(this->mpGraphicsDevice);
	this->mDepthView
		.setImage(&this->mDepthImage, vk::ImageAspectFlagBits::eDepth)
		.create();
}

void GameRenderer::destroyDepthResources()
{
	this->mDepthView.invalidate();
	this->mDepthImage.invalidate();
	this->mpMemoryDepthImage.reset();
}

void GameRenderer::createDescriptors()
{	
	auto& samplerPair = this->mTextureDescriptorPairs[0];
	this->mDescriptorGroup
		.setBindingCount(2)
		.setAmount((ui32)this->mFrameImageViews.size())
		.addBinding(0, vk::DescriptorType::eUniformBuffer, vk::ShaderStageFlagBits::eVertex)
		.attachToBinding(0, this->mUniformStaticBuffersPerFrame, 0)
		.addBinding(1, vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eFragment)
		.attachToBinding(1, vk::ImageLayout::eShaderReadOnlyOptimal, &this->mTextureViews[samplerPair.first], &this->mTextureSamplers[samplerPair.second])
		.create(this->mpGraphicsDevice, &this->mDescriptorPool)
		.writeAttachments(this->mpGraphicsDevice);
	auto& font = this->stringRenderer()->getFont();
	this->mDescriptorGroupUI
		.setBindingCount(1)
		.setAmount((ui32)this->mFrameImageViews.size())
		.addBinding(0, vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eFragment)
		.attachToBinding(0, vk::ImageLayout::eShaderReadOnlyOptimal, &font.faces()[0].view(), &font.faces()[0].sampler())
		.create(this->mpGraphicsDevice, &this->mDescriptorPool)
		.writeAttachments(this->mpGraphicsDevice);
}

void GameRenderer::createCommandObjects()
{
	auto resolution = this->mSwapChain.getResolution();

	{
		auto viewCount = this->mFrameImageViews.size();
		this->mFrameBuffers.resize(viewCount);
		for (uSize i = 0; i < viewCount; ++i)
		{
			this->mFrameBuffers[i]
				.setRenderPass(&this->mRenderPass)
				.setResolution(this->mSwapChain.getResolution())
				.addAttachment(&this->mFrameImageViews[i])
				.addAttachment(&this->mDepthView)
				.create(this->mpGraphicsDevice);
		}
	}

	// color = (newColor.alpha * newColor.rgb) + ((1 - newColor.alpha) * oldColor.rgb)
	// alpha = (1 * newColor.alpha) + (0 * oldColor.alpha)
	BlendMode overlayBlendMode;
	overlayBlendMode.writeMask |= graphics::ColorComponent::Enum::eR;
	overlayBlendMode.writeMask |= graphics::ColorComponent::Enum::eG;
	overlayBlendMode.writeMask |= graphics::ColorComponent::Enum::eB;
	overlayBlendMode.writeMask |= graphics::ColorComponent::Enum::eA;
	overlayBlendMode.blend = {
		{ graphics::BlendOperation::Enum::eAdd, graphics::BlendFactor::Enum::eSrcAlpha, graphics::BlendFactor::Enum::eOneMinusSrcAlpha },
		{ graphics::BlendOperation::Enum::eAdd, graphics::BlendFactor::Enum::eOne, graphics::BlendFactor::Enum::eZero }
	};

	auto& fullViewport = vk::Viewport()
		.setX(0).setY(0)
		.setWidth((f32)resolution.x()).setHeight((f32)resolution.y())
		.setMinDepth(0.0f).setMaxDepth(1.0f);
	this->mPipeline.setDevice(this->mpGraphicsDevice);
	this->mPipeline
		.setViewArea(fullViewport, vk::Rect2D().setOffset({ 0, 0 }).setExtent({ resolution.x(), resolution.y() }))
		.setFrontFace(vk::FrontFace::eCounterClockwise)
		.setRenderPass(&this->mRenderPass).setDescriptors({ &this->mDescriptorGroup })
		.create();
	this->mPipelineUI.setDevice(this->mpGraphicsDevice);
	this->mPipelineUI
		.setViewArea(fullViewport, vk::Rect2D().setOffset({ 0, 0 }).setExtent({ resolution.x(), resolution.y() }))
		.setFrontFace(vk::FrontFace::eClockwise)
		.setBlendMode(overlayBlendMode)
		.setRenderPass(&this->mRenderPass).setDescriptors({ &this->mDescriptorGroupUI })
		.create();

	this->mCommandPool.setDevice(this->mpGraphicsDevice);
	this->mCommandPool
		.setQueueFamily(graphics::QueueFamily::Enum::eGraphics, this->mpGraphicsDevice->queryQueueFamilyGroup())
		.create();
	this->mCommandBuffers = this->mCommandPool.createCommandBuffers((ui32)this->mFrameImageViews.size());

	this->recordCommandBufferInstructions();
}

void GameRenderer::recordCommandBufferInstructions()
{
	for (uIndex idxFrame = 0; idxFrame < this->mCommandBuffers.size(); ++idxFrame)
	{
		auto cmd = this->mCommandBuffers[idxFrame].beginCommand();
		// Clear commands MUST be passed in the same order as the attachments on framebuffers
		cmd.clearColor({ 0.0f, 0.0f, 0.0f, 1.0f });
		cmd.clearDepth(1.0f, 0);
		cmd.setRenderArea({ 0, 0 }, this->mSwapChain.getResolution());
		cmd.beginRenderPass(&this->mRenderPass, &this->mFrameBuffers[idxFrame]);
		{
			// TODO: Should each render system have control over the descriptor set and pipeline?
			cmd.bindDescriptorSet(&this->mPipeline, &this->mDescriptorGroup[idxFrame]);
			cmd.bindPipeline(&this->mPipeline);
			for (auto* pRender : this->mpRenders)
			{
				pRender->draw(&cmd);
			}

			cmd.bindDescriptorSet(&this->mPipelineUI, &this->mDescriptorGroupUI[idxFrame]);
			cmd.bindPipeline(&this->mPipelineUI);
			cmd.bindVertexBuffers(0, { &this->mVertexBufferUI });
			cmd.bindIndexBuffer(0, &this->mIndexBufferUI, vk::IndexType::eUint16);
			cmd.draw((ui32)this->mIndexBufferUI.getSize() / sizeof(ui16), 1);
		}
		cmd.endRenderPass();
		cmd.end();
	}
}

void GameRenderer::destroyCommandObjects()
{
	this->mCommandPoolTransient.destroy();
	this->mCommandBuffers.clear();
	this->mCommandPool.destroy();
	this->mPipeline.destroy();
	this->mPipelineUI.destroy();
	this->mFrameBuffers.clear();
}

void GameRenderer::createFrames(uSize viewCount)
{
	this->mFrames.resize(viewCount);
	for (auto& frame : this->mFrames)
	{
		frame.create(this->mpGraphicsDevice);
	}
}

uSize GameRenderer::getNumberOfFrames() const
{
	return this->mFrames.size();
}

graphics::Frame* GameRenderer::getFrameAt(uSize idx)
{
	return &this->mFrames[idx];
}

void GameRenderer::destroyFrames()
{
	this->mFrames.clear();
}

void GameRenderer::prepareRender(ui32 idxCurrentFrame)
{
	VulkanRenderer::prepareRender(idxCurrentFrame);
	this->updateUniformBuffer(idxCurrentFrame);
}

void GameRenderer::updateUniformBuffer(ui32 idxImageView)
{
	this->mpUniformStatic->beginReading();
	this->mUniformStaticBuffersPerFrame[idxImageView].write(
		/*offset*/ 0, this->mpUniformStatic->data(), this->mpUniformStatic->size()
	);
	this->mpUniformStatic->endReading();
}

void GameRenderer::render(graphics::Frame* currentFrame, ui32 idxCurrentImage)
{
	// Submit the command buffer to the graphics queue
	auto& commandBuffer = this->mCommandBuffers[idxCurrentImage];
	currentFrame->submitBuffers(&this->getQueue(QueueFamily::Enum::eGraphics), { &commandBuffer });
}

void GameRenderer::onFramePresented(uIndex idxFrame)
{
	// TODO: Copy all dirty strings into UI buffer
	auto strDrawer = this->stringRenderer();
	if (strDrawer->isDirty())
	{
		this->mIndexCountUI = strDrawer->writeBuffers(&this->mCommandPoolTransient, &this->mVertexBufferUI, &this->mIndexBufferUI);
	}
}
