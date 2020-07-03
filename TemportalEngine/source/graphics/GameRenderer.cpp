#include "graphics/GameRenderer.hpp"

#include "IRender.hpp"
#include "asset/Font.hpp"
#include "asset/Texture.hpp"
#include "asset/TextureSampler.hpp"
#include "graphics/Uniform.hpp"
#include "graphics/VulkanApi.hpp"

using namespace graphics;

GameRenderer::GameRenderer()
	: VulkanRenderer()
{
	this->mDescriptorPool.setPoolSize(64, {
		{ vk::DescriptorType::eUniformBuffer, 64 },
		{ vk::DescriptorType::eCombinedImageSampler, 64 },
	});
	this->mVertexBufferUI
		.setUsage(vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eVertexBuffer)
		.setMemoryRequirements(vk::MemoryPropertyFlagBits::eDeviceLocal);
	this->mIndexBufferUI
		.setUsage(vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eIndexBuffer)
		.setMemoryRequirements(vk::MemoryPropertyFlagBits::eDeviceLocal);
}

void GameRenderer::invalidate()
{
	this->mTextureDescriptorPairs.clear();
	this->mTextureViews.clear();
	this->mTextureImages.clear();
	this->mTextureSamplers.clear();

	this->mFont.invalidate();

	this->destroyRenderChain();
	this->mPipeline.clearShaders();
	this->mPipelineUI.clearShaders();

	this->mVertexBufferUI.destroy();
	this->mIndexBufferUI.destroy();

	VulkanRenderer::invalidate();
}

void GameRenderer::initializeDevices()
{
	VulkanRenderer::initializeDevices();
	this->initializeTransientCommandPool();
}

void GameRenderer::addRender(IRender *render)
{
	this->mpRenders.push_back(render);
}

void GameRenderer::setStaticUniform(std::shared_ptr<Uniform> uniform)
{
	this->mpUniformStatic = uniform;
}

void GameRenderer::initializeTransientCommandPool()
{
	this->mCommandPoolTransient
		.setQueueFamily(
			graphics::QueueFamily::Enum::eGraphics,
			this->mPhysicalDevice.queryQueueFamilyGroup()
		)
		.create(&this->mLogicalDevice, vk::CommandPoolCreateFlagBits::eTransient);
}

void GameRenderer::initializeBuffer(graphics::Buffer &buffer)
{
	buffer.create(&this->mLogicalDevice);
}

void GameRenderer::writeToBuffer(Buffer* buffer, uSize offset, void* data, uSize size)
{
	Buffer& stagingBuffer = Buffer()
		.setUsage(vk::BufferUsageFlagBits::eTransferSrc)
		.setSize(size);
	stagingBuffer.setMemoryRequirements(
		vk::MemoryPropertyFlagBits::eHostVisible
		| vk::MemoryPropertyFlagBits::eHostCoherent
	);
	stagingBuffer.create(&this->mLogicalDevice);
	stagingBuffer.write(&this->mLogicalDevice, offset, data, size);
	this->copyBetweenBuffers(&stagingBuffer, buffer, size);
	stagingBuffer.destroy();
}

void GameRenderer::copyBetweenBuffers(Buffer *src, Buffer *dest, uSize size)
{
	// Buffers should be freed when they go out of scope
	auto buffers = this->mCommandPoolTransient.createCommandBuffers(1);
	buffers[0]
		.beginCommand(vk::CommandBufferUsageFlagBits::eOneTimeSubmit)
		.copyBuffer(src, dest, size)
		.end();
	auto queue = this->mQueues[QueueFamily::Enum::eGraphics];
	queue.submit(
		vk::SubmitInfo()
		.setCommandBufferCount(1)
		.setPCommandBuffers(&extract<vk::CommandBuffer>(&buffers[0])),
		vk::Fence()
	);
	queue.waitIdle();
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

uIndex GameRenderer::createTextureSampler(std::shared_ptr<asset::TextureSampler> sampler)
{
	auto addressModes = sampler->getAddressModes();
	auto compareOp = sampler->getCompareOperation();
	uIndex idx = this->mTextureSamplers.size();
	this->mTextureSamplers.push_back(graphics::ImageSampler());
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
	this->mTextureSamplers[idx].create(&this->mLogicalDevice);
	return idx;
}

uIndex GameRenderer::createTextureAssetImage(std::shared_ptr<asset::Texture> texture, uIndex idxSampler)
{
	std::vector<ui8> data = texture->getSourceBinary();
	auto dataMemSize = data.size() * sizeof(ui8);

	auto idxImage = this->mTextureImages.size();
	this->mTextureImages.push_back(graphics::Image());
	this->mTextureImages[idxImage]
		.setFormat(vk::Format::eR8G8B8A8Srgb)
		.setSize(math::Vector3UInt(texture->getSourceSize()).z(1))
		.setTiling(vk::ImageTiling::eOptimal)
		.setUsage(vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled);
	this->mTextureImages[idxImage].setMemoryRequirements(vk::MemoryPropertyFlagBits::eDeviceLocal);
	this->mTextureImages[idxImage].create(&this->mLogicalDevice);

	this->transitionImageToLayout(&this->mTextureImages[idxImage], vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal);
	{
		Buffer& stagingBuffer = Buffer()
			.setUsage(vk::BufferUsageFlagBits::eTransferSrc)
			.setSize(this->mTextureImages[idxImage].getMemorySize());
		stagingBuffer.setMemoryRequirements(
			vk::MemoryPropertyFlagBits::eHostVisible
			| vk::MemoryPropertyFlagBits::eHostCoherent
		);
		stagingBuffer.create(&this->mLogicalDevice);
		stagingBuffer.write(&this->mLogicalDevice, /*offset*/ 0, (void*)data.data(), dataMemSize);
		this->copyBufferToImage(&stagingBuffer, &this->mTextureImages[idxImage]);
		stagingBuffer.destroy();
	}
	this->transitionImageToLayout(&this->mTextureImages[idxImage], vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eShaderReadOnlyOptimal);

	uIndex idx = this->mTextureViews.size();
	this->mTextureViews.push_back(graphics::ImageView());
	this->mTextureViews[idx].setImage(&this->mTextureImages[idxImage], vk::ImageAspectFlagBits::eColor).create(&this->mLogicalDevice);

	this->mTextureDescriptorPairs.push_back(std::make_pair(idx, idxSampler));

	return idx;
}

void GameRenderer::copyBufferToImage(Buffer *src, Image *dest)
{
	auto buffers = this->mCommandPoolTransient.createCommandBuffers(1);
	buffers[0]
		.beginCommand(vk::CommandBufferUsageFlagBits::eOneTimeSubmit)
		.copyBufferToImage(src, dest)
		.end();
	auto queue = this->mQueues[QueueFamily::Enum::eGraphics];
	queue.submit(
		vk::SubmitInfo()
		.setCommandBufferCount(1)
		.setPCommandBuffers(&extract<vk::CommandBuffer>(&buffers[0])),
		vk::Fence()
	);
	/*
		TODO: This causes all commands to be synchronous.
		For higher throughput, this should not be called, and the command buffers should rely entirely
		on the pipeline barriers.
		This means multiple images could be sent to GPU at once.
	*/
	queue.waitIdle();
}

void GameRenderer::transitionImageToLayout(Image *image, vk::ImageLayout prev, vk::ImageLayout next)
{
	auto buffers = this->mCommandPoolTransient.createCommandBuffers(1);
	buffers[0]
		.beginCommand(vk::CommandBufferUsageFlagBits::eOneTimeSubmit)
		.setPipelineImageBarrier(image, prev, next)
		.end();
	auto queue = this->mQueues[QueueFamily::Enum::eGraphics];
	queue.submit(
		vk::SubmitInfo()
		.setCommandBufferCount(1)
		.setPCommandBuffers(&extract<vk::CommandBuffer>(&buffers[0])),
		vk::Fence()
	);
	/*
		TODO: This causes all commands to be synchronous.
		For higher throughput, this should not be called, and the command buffers should rely entirely
		on the pipeline barriers.
		This means multiple images could be sent to GPU at once.
	*/
	queue.waitIdle();
}

void GameRenderer::setFont(std::shared_ptr<asset::Font> font)
{
	this->mFont.loadGlyphSets(font->getFontSizes(), font->glyphSets());
	for (auto& face : this->mFont.faces())
	{
		face.sampler()
			.setFilter(vk::Filter::eLinear, vk::Filter::eLinear)
			.setAddressMode({ vk::SamplerAddressMode::eClampToEdge, vk::SamplerAddressMode::eClampToEdge, vk::SamplerAddressMode::eClampToEdge })
			.setAnistropy(std::nullopt)
			.setBorderColor(vk::BorderColor::eIntOpaqueBlack)
			.setNormalizeCoordinates(true)
			.setCompare(std::nullopt)
			.setMipLOD(vk::SamplerMipmapMode::eNearest, 0, { 0, 0 })
			.create(&this->mLogicalDevice);
		
		auto& image = face.image();
		image
			.setFormat(vk::Format::eR8G8B8A8Srgb)
			.setSize(math::Vector3UInt(face.getAtlasSize()).z(1))
			.setTiling(vk::ImageTiling::eOptimal)
			.setUsage(vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled);
		image.setMemoryRequirements(vk::MemoryPropertyFlagBits::eDeviceLocal);
		image.create(&this->mLogicalDevice);

		auto& data = face.getPixelData();
		auto dataMemSize = data.size() * sizeof(ui8);

		this->transitionImageToLayout(&image, vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal);
		{
			Buffer& stagingBuffer = Buffer()
				.setUsage(vk::BufferUsageFlagBits::eTransferSrc)
				.setSize(image.getMemorySize());
			stagingBuffer.setMemoryRequirements(
				vk::MemoryPropertyFlagBits::eHostVisible
				| vk::MemoryPropertyFlagBits::eHostCoherent
			);
			stagingBuffer.create(&this->mLogicalDevice);
			stagingBuffer.write(&this->mLogicalDevice, /*offset*/ 0, (void*)data.data(), dataMemSize);
			this->copyBufferToImage(&stagingBuffer, &image);
			stagingBuffer.destroy();
		}
		this->transitionImageToLayout(&image, vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eShaderReadOnlyOptimal);

		face.view()
			.setImage(&image, vk::ImageAspectFlagBits::eColor)
			.create(&this->mLogicalDevice);
	}
}

void GameRenderer::createRenderChain()
{
	if (!this->mCommandPoolTransient.isValid())
	{
		this->initializeTransientCommandPool();
	}

	this->createSwapChain();
	this->createFrameImageViews();
	this->createDepthResources(this->mSwapChain.getResolution());
	this->createRenderPass();

	this->createUniformBuffers();
	this->mDescriptorPool.create(&this->mLogicalDevice, (ui32)this->mFrameImageViews.size());
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
	this->destroyUniformBuffers();

	this->destroyRenderPass();
	this->destroyDepthResources();
	this->destroyFrameImageViews();
	this->destroySwapChain();

	this->mCommandPoolTransient.destroy();
}

void GameRenderer::createRenderPass()
{
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

	this->mRenderPass.create(&this->mLogicalDevice);
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
	auto frameCount = this->mFrameImageViews.size();
	this->mUniformStaticBuffersPerFrame.resize(frameCount);
	for (ui32 idxFrame = 0; idxFrame < frameCount; ++idxFrame)
	{
		this->mUniformStaticBuffersPerFrame[idxFrame]
			.setUsage(vk::BufferUsageFlagBits::eUniformBuffer)
			.setSize(this->mpUniformStatic->size());
		this->mUniformStaticBuffersPerFrame[idxFrame]
			// Host Coherent means this entire buffer will be automatically flushed per write.
			// This can be optimized later by only flushing the portion of the buffer which actually changed.
			.setMemoryRequirements(vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);
		this->mUniformStaticBuffersPerFrame[idxFrame].create(&this->mLogicalDevice);
	}
}

void GameRenderer::destroyUniformBuffers()
{
	this->mUniformStaticBuffersPerFrame.clear();
}

void GameRenderer::createDepthResources(math::Vector2UInt const &resolution)
{

	auto supportedFormat = this->mPhysicalDevice.pickFirstSupportedFormat(
		{ vk::Format::eD32Sfloat, vk::Format::eD32SfloatS8Uint, vk::Format::eD24UnormS8Uint },
		vk::ImageTiling::eOptimal, vk::FormatFeatureFlagBits::eDepthStencilAttachment
	);
	if (!supportedFormat) throw std::runtime_error("failed to find supported depth buffer format");

	this->mDepthImage.setMemoryRequirements(vk::MemoryPropertyFlagBits::eDeviceLocal);
	this->mDepthImage
		.setFormat(*supportedFormat)
		.setSize({ resolution.x(), resolution.y(), 1 })
		.setTiling(vk::ImageTiling::eOptimal)
		.setUsage(vk::ImageUsageFlagBits::eDepthStencilAttachment)
		.create(&this->mLogicalDevice);

	this->transitionImageToLayout(&this->mDepthImage, vk::ImageLayout::eUndefined, vk::ImageLayout::eDepthStencilAttachmentOptimal);

	this->mDepthView
		.setImage(&this->mDepthImage, vk::ImageAspectFlagBits::eDepth)
		.create(&this->mLogicalDevice);
}

void GameRenderer::destroyDepthResources()
{
	this->mDepthView.invalidate();
	this->mDepthImage.invalidate();
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
		.create(&this->mLogicalDevice, &this->mDescriptorPool)
		.writeAttachments(&this->mLogicalDevice);
	this->mDescriptorGroupUI
		.setBindingCount(1)
		.setAmount((ui32)this->mFrameImageViews.size())
		.addBinding(0, vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eFragment)
		.attachToBinding(0, vk::ImageLayout::eShaderReadOnlyOptimal, &this->mFont.faces()[0].view(), &this->mFont.faces()[0].sampler())
		.create(&this->mLogicalDevice, &this->mDescriptorPool)
		.writeAttachments(&this->mLogicalDevice);
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
				.create(&this->mLogicalDevice);
		}
	}

	auto& fullViewport = vk::Viewport()
		.setX(0).setY(0)
		.setWidth((f32)resolution.x()).setHeight((f32)resolution.y())
		.setMinDepth(0.0f).setMaxDepth(1.0f);
	this->mPipeline
		.setViewArea(fullViewport, vk::Rect2D().setOffset({ 0, 0 }).setExtent({ resolution.x(), resolution.y() }))
		.setFrontFace(vk::FrontFace::eCounterClockwise)
		.create(&this->mLogicalDevice, &this->mRenderPass, { &this->mDescriptorGroup });
	this->mPipelineUI
		.setViewArea(fullViewport, vk::Rect2D().setOffset({ 0, 0 }).setExtent({ resolution.x(), resolution.y() }))
		.setFrontFace(vk::FrontFace::eClockwise)
		.create(&this->mLogicalDevice, &this->mRenderPass, { &this->mDescriptorGroupUI });

	this->mCommandPool
		.setQueueFamily(graphics::QueueFamily::Enum::eGraphics, mPhysicalDevice.queryQueueFamilyGroup())
		.create(&this->mLogicalDevice);
	this->mCommandBuffers = this->mCommandPool.createCommandBuffers(this->mFrameImageViews.size());

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
			cmd.draw(6, 1);
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
		frame.create(&this->mLogicalDevice);
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
	/*
	ui64 offset = 0;
	for (auto& uniformPtr : this->mUniformPts)
	{
		auto uniformSize = uniformPtr->size();
		uniformPtr->beginReading();
		this->mUniformBuffers[idxImageView].write(&this->mLogicalDevice, offset, uniformPtr->data(), uniformSize);
		uniformPtr->endReading();
		offset += uniformSize;
	}
	//*/
	this->mpUniformStatic->beginReading();
	this->mUniformStaticBuffersPerFrame[idxImageView].write(&this->mLogicalDevice,
		/*offset*/ 0, this->mpUniformStatic->data(), this->mpUniformStatic->size()
	);
	this->mpUniformStatic->endReading();
}

void GameRenderer::render(graphics::Frame* currentFrame, ui32 idxCurrentImage)
{
	// Submit the command buffer to the graphics queue
	auto& commandBuffer = this->mCommandBuffers[idxCurrentImage];
	currentFrame->submitBuffers(&this->mQueues[QueueFamily::Enum::eGraphics], { &commandBuffer });
}
