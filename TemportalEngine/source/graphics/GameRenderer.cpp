#include "graphics/GameRenderer.hpp"

#include "IRender.hpp"
#include "asset/Font.hpp"
#include "asset/PipelineAsset.hpp"
#include "asset/RenderPassAsset.hpp"
#include "asset/Shader.hpp"
#include "asset/Texture.hpp"
#include "asset/TextureSampler.hpp"
#include "asset/TypedAssetPath.hpp"
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

	for (auto& pipelineSet : this->mPipelineSets) pipelineSet.pipeline->destroyShaders();
	this->mPipelineSets.clear();

	this->mUniformStaticBuffersPerFrame.clear();
	this->mpMemoryUniformBuffers->destroy();

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

void GameRenderer::setBindings(uIndex const &idxPipeline, std::vector<AttributeBinding> bindings)
{
	this->mPipelineSets[idxPipeline].pipeline->setBindings(bindings);
}

void GameRenderer::setRenderPass(std::shared_ptr<asset::RenderPass> asset)
{
	// TODO: Don't use default make_shared
	this->mpRenderPass = std::make_shared<graphics::RenderPass>();
	this->mpRenderPass->setDevice(this->mpGraphicsDevice);

	this->mpRenderPass->setClearColor(asset->getClearColorValue());
	this->mpRenderPass->setClearDepthStencil(asset->getDepthStencilClearValues());
	this->mpRenderPass->setRenderArea(asset->getRenderArea());

	for (auto phase : asset->getPhases())
	{
		this->mpRenderPass->addPhase(phase);
	}
	for (auto dependency : asset->getPhaseDependencies())
	{
		this->mpRenderPass->addDependency(dependency);
	}

	// TODO: All pipelines need a string key to be identified by
	for (auto &typedAssetPath : asset->getPipelineRefs())
	{
		RenderPassPipeline pipelineSet;

		// Perform a synchronous load to fetch the asset
		auto pipelineAsset = typedAssetPath.load(asset::EAssetSerialization::Binary);

		// TODO: Don't use default make_shared
		pipelineSet.pipeline = std::make_shared<graphics::Pipeline>();
		pipelineSet.pipeline->setDevice(this->mpGraphicsDevice);
		pipelineSet.pipeline->setRenderPass(this->mpRenderPass);
		
		// TODO: offset and size will need to be scaled by current frame resolution
		pipelineSet.pipeline->addViewArea(pipelineAsset->getViewport(), pipelineAsset->getScissor());
		pipelineSet.pipeline->setBlendMode(pipelineAsset->getBlendMode());
		pipelineSet.pipeline->setFrontFace(pipelineAsset->getFrontFace());
		pipelineSet.pipeline->setTopology(pipelineAsset->getTopology());
		
		// Perform a synchronous load on each shader to create the shader modules
		pipelineSet.pipeline->addShader(pipelineAsset->getVertexShader().load(asset::EAssetSerialization::Binary)->makeModule());
		pipelineSet.pipeline->addShader(pipelineAsset->getFragmentShader().load(asset::EAssetSerialization::Binary)->makeModule());

		// Create descriptor groups for the pipeline
		for (auto const& assetDescGroup : pipelineAsset->getDescriptorGroups())
		{
			auto const& descriptors = assetDescGroup.descriptors;
			auto descriptorGroup = DescriptorGroup();
			descriptorGroup.setBindingCount(descriptors.size());
			for (uIndex i = 0; i < descriptors.size(); ++i)
			{
				descriptorGroup.addBinding(descriptors[i].id, i, descriptors[i].type, descriptors[i].stage);
			}
			pipelineSet.descriptorGroups.push_back(std::move(descriptorGroup));
		}

		pipelineSet.pipeline->setDescriptors(&pipelineSet.descriptorGroups);

		this->mPipelineSets.push_back(std::move(pipelineSet));
	}
}

uIndex GameRenderer::createTextureSampler(std::shared_ptr<asset::TextureSampler> sampler)
{
	uIndex idx = this->mTextureSamplers.size();
	this->mTextureSamplers.push_back(graphics::ImageSampler());
	this->createTextureSampler(sampler, &this->mTextureSamplers[idx]);
	return idx;
}

void GameRenderer::createTextureSampler(std::shared_ptr<asset::TextureSampler> sampler, graphics::ImageSampler *out)
{
	out->setDevice(this->mpGraphicsDevice);
	out
		->setFilter(
			sampler->getFilterModeMagnified(),
			sampler->getFilterModeMinified()
		)
		.setAddressMode(sampler->getAddressModes())
		.setAnistropy(sampler->getAnisotropy())
		.setBorderColor(sampler->getBorderColor())
		.setNormalizeCoordinates(sampler->areCoordinatesNormalized())
		.setCompare(sampler->getCompareOperation())
		.setMipLOD(
			sampler->getLodMode(),
			sampler->getLodBias(), sampler->getLodRange()
		);
	out->create();
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
			.setMipLOD(graphics::SamplerLODMode::Enum::Nearest, 0, { 0, 0 })
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
	OPTICK_EVENT();
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
	OPTICK_EVENT();
	this->destroyFrames();

	for (auto& pipelineSet : this->mPipelineSets)
	{
		pipelineSet.pipeline->invalidate();
		for (auto& descriptorGroup : pipelineSet.descriptorGroups)
		{
			descriptorGroup.invalidate();
		}
	}

	this->destroyCommandObjects();
	this->mDescriptorPool.invalidate();

	this->destroyRenderPass();
	this->destroyDepthResources();
	this->destroyFrameImageViews();
	this->destroySwapChain();

	this->mCommandPoolTransient.destroy();
}

void GameRenderer::createRenderPass()
{
	this->mpRenderPass->setImageFormatType(graphics::ImageFormatReferenceType::Enum::Viewport, (ui32)this->mSwapChain.getFormat());
	this->mpRenderPass->setImageFormatType(graphics::ImageFormatReferenceType::Enum::Depth, (ui32)this->mDepthImage.getFormat());
	this->mpRenderPass->create();
}

RenderPass* GameRenderer::getRenderPass()
{
	return this->mpRenderPass.get();
}

void GameRenderer::destroyRenderPass()
{
	this->mpRenderPass->destroy();
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
	auto const frameImages = (ui32)this->mFrameImageViews.size();

	this->mPipelineSets[0].descriptorGroups[0].setAmount(frameImages);
	this->mPipelineSets[0].descriptorGroups[1].setAmount(1);
	this->mPipelineSets[1].descriptorGroups[0].setAmount(frameImages);

	// 0 = WorldPipeline: 0 = Only DescriptorGroup
	//this->mPipelineSets[0].descriptorGroups[0]
	//	.attachToBinding("mvpUniform", this->mUniformStaticBuffersPerFrame);
	this->mPipelineSets[0].descriptorGroups[1]
		.attachToBinding("imageAtlas", vk::ImageLayout::eShaderReadOnlyOptimal, &this->mTextureViews[samplerPair.first], &this->mTextureSamplers[samplerPair.second]);
	
	auto& font = this->stringRenderer()->getFont();
	// 1 = UIPipeline: 0 = Only DescriptorGroup
	this->mPipelineSets[1].descriptorGroups[0]
		.attachToBinding("fontAtlas", vk::ImageLayout::eShaderReadOnlyOptimal, &font.faces()[0].view(), &font.faces()[0].sampler());
	
	for (auto& pipelineSet : this->mPipelineSets)
	{
		for (auto& descriptorGroup : pipelineSet.descriptorGroups)
		{
			descriptorGroup
				.create(this->mpGraphicsDevice, &this->mDescriptorPool)
				.writeAttachments(this->mpGraphicsDevice);
		}
	}
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
				.setRenderPass(this->getRenderPass())
				.setResolution(this->mSwapChain.getResolution())
				.addAttachment(&this->mFrameImageViews[i])
				.addAttachment(&this->mDepthView)
				.create(this->mpGraphicsDevice);
		}
	}

	for (auto& pipelineSet : this->mPipelineSets)
	{
		pipelineSet.pipeline->setResolution(resolution);
		pipelineSet.pipeline->setDescriptors(&pipelineSet.descriptorGroups);
		pipelineSet.pipeline->create();
	}

	this->mCommandPool.setDevice(this->mpGraphicsDevice);
	this->mCommandPool
		.setQueueFamily(graphics::QueueFamily::Enum::eGraphics, this->mpGraphicsDevice->queryQueueFamilyGroup())
		.create();
	this->mCommandBuffers = this->mCommandPool.createCommandBuffers((ui32)this->mFrameImageViews.size());

	this->recordCommandBufferInstructions();
}

bool GameRenderer::renderCommandReRecordRequired() const
{
	for (auto* pRender : this->mpRenders)
	{
		if (pRender->reRecordRequired()) return true;
	}
	return false;
}

void GameRenderer::resetCommandBuffer()
{
	this->mCommandPool.resetPool();
}

void GameRenderer::recordCommandBufferInstructions()
{
	for (uIndex idxFrame = 0; idxFrame < this->mCommandBuffers.size(); ++idxFrame)
	{
		auto cmd = this->mCommandBuffers[idxFrame].beginCommand();
		cmd.beginRenderPass(this->getRenderPass(), &this->mFrameBuffers[idxFrame], this->mSwapChain.getResolution());
		{
			cmd.bindDescriptorSets(
				this->mPipelineSets[0].pipeline,
				{
					this->mPipelineSets[0].descriptorGroups[0].getDescriptorSet(idxFrame),
					this->mPipelineSets[0].descriptorGroups[1].getDescriptorSet(0)
				}
			);
			cmd.bindPipeline(this->mPipelineSets[0].pipeline);
			for (auto* pRender : this->mpRenders)
			{
				pRender->record(&cmd);
			}

			cmd.bindDescriptorSets(this->mPipelineSets[1].pipeline, { this->mPipelineSets[1].descriptorGroups[0].getDescriptorSet(idxFrame) });
			cmd.bindPipeline(this->mPipelineSets[1].pipeline);
			cmd.bindVertexBuffers(0, { &this->mVertexBufferUI });
			cmd.bindIndexBuffer(0, &this->mIndexBufferUI, vk::IndexType::eUint16);
			cmd.draw(0, (ui32)this->mIndexBufferUI.getSize() / sizeof(ui16), 0, 0, 1);
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
	this->mpUniformStatic->endReading(true);
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

	if (this->renderCommandReRecordRequired())
	{
		this->resetCommandBuffer();
		this->recordCommandBufferInstructions();
	}
}
