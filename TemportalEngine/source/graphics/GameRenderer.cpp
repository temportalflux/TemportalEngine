#include "graphics/GameRenderer.hpp"

#include "IRender.hpp"
#include "asset/Texture.hpp"
#include "asset/TextureSampler.hpp"
#include "graphics/Uniform.hpp"

using namespace graphics;

GameRenderer::GameRenderer()
	: VulkanRenderer()
{
}

void GameRenderer::invalidate()
{
	this->mTextureDescriptorPairs.clear();
	this->mTextureViews.clear();
	this->mTextureImages.clear();
	this->mTextureSamplers.clear();

	this->destroyRenderChain();
	this->mPipeline.clearShaders();
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
		.setPCommandBuffers(reinterpret_cast<vk::CommandBuffer*>(buffers[0].get())),
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
		.setPCommandBuffers(reinterpret_cast<vk::CommandBuffer*>(buffers[0].get())),
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
		.setPCommandBuffers(reinterpret_cast<vk::CommandBuffer*>(buffers[0].get())),
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

void GameRenderer::createRenderChain()
{
	if (!this->mCommandPoolTransient.isValid())
	{
		this->initializeTransientCommandPool();
	}

	this->createSwapChain();
	this->createFrameImageViews();
	this->createDepthResources(this->mSwapChain.getResolution());
	this->createRenderPass(this->mDepthImage.getFormat());

	this->createUniformBuffers();
	this->createDescriptorPool();
	this->createCommandObjects();

	this->createFrames(this->mFrameImageViews.size());
}

void GameRenderer::destroyRenderChain()
{
	this->destroyFrames();

	this->destroyCommandObjects();
	this->destroyDescriptorPool();
	this->destroyUniformBuffers();

	this->destroyRenderPass();
	this->destroyDepthResources();
	this->destroyFrameImageViews();
	this->destroySwapChain();

	this->mCommandPoolTransient.destroy();
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
			// Host Coherent means this entire buffer will be automatially flushed per write.
			// This can be optimized later by only flushing the portion of the buffer which actually changed.
			.setMemoryRequirements(vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);
		this->mUniformStaticBuffersPerFrame[idxFrame].create(&this->mLogicalDevice);
	}
}

void GameRenderer::destroyUniformBuffers()
{
	this->mUniformStaticBuffersPerFrame.clear();
}

void GameRenderer::createDepthResources(vk::Extent2D const &resolution)
{

	auto supportedFormat = this->mPhysicalDevice.pickFirstSupportedFormat(
		{ vk::Format::eD32Sfloat, vk::Format::eD32SfloatS8Uint, vk::Format::eD24UnormS8Uint },
		vk::ImageTiling::eOptimal, vk::FormatFeatureFlagBits::eDepthStencilAttachment
	);
	if (!supportedFormat) throw std::runtime_error("failed to find supported depth buffer format");

	this->mDepthImage.setMemoryRequirements(vk::MemoryPropertyFlagBits::eDeviceLocal);
	this->mDepthImage
		.setFormat(*supportedFormat)
		.setSize({ resolution.width, resolution.height, 1 })
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

void GameRenderer::createDescriptorPool()
{
	auto frameCount = (ui32)this->mFrameImageViews.size();
	
	auto uniformDescriptorCount = 1;
	auto& uniformBufferPerFrame = this->mUniformStaticBuffersPerFrame;
	auto uniformBufferRange = this->mpUniformStatic->size();

	ui32 samplerDescriptorCount = (ui32)this->mTextureSamplers.size();

	std::vector<vk::DescriptorPoolSize> poolSizes = {
		// Uniform Buffer Pool size
		vk::DescriptorPoolSize()
		.setType(vk::DescriptorType::eUniformBuffer)
		.setDescriptorCount(frameCount * uniformDescriptorCount),
		// Texture Images
		vk::DescriptorPoolSize()
		.setType(vk::DescriptorType::eCombinedImageSampler)
		.setDescriptorCount(frameCount * samplerDescriptorCount)
	};
	this->mDescriptorPool = this->mLogicalDevice.mDevice->createDescriptorPoolUnique(
		vk::DescriptorPoolCreateInfo()
		.setPoolSizeCount((ui32)poolSizes.size()).setPPoolSizes(poolSizes.data())
		.setMaxSets(frameCount)
	);

	std::vector<vk::DescriptorSetLayoutBinding> bindings = {
		// Uniform Buffer Binding
		vk::DescriptorSetLayoutBinding()
		.setBinding(/*index of binding in bindings*/ 0)
		.setDescriptorType(vk::DescriptorType::eUniformBuffer)
		.setDescriptorCount(uniformDescriptorCount)
		.setStageFlags(vk::ShaderStageFlagBits::eVertex),
		// Texture Images
		vk::DescriptorSetLayoutBinding()
		.setBinding(/*index of binding in bindings*/ 1)
		.setDescriptorType(vk::DescriptorType::eCombinedImageSampler)
		.setDescriptorCount(samplerDescriptorCount)
		.setStageFlags(vk::ShaderStageFlagBits::eFragment)
	};
	this->mDescriptorLayout = this->mLogicalDevice.mDevice->createDescriptorSetLayoutUnique(
		vk::DescriptorSetLayoutCreateInfo()
		.setBindingCount((ui32)bindings.size()).setPBindings(bindings.data())
	);

	std::vector<vk::DescriptorSetLayout> layouts(frameCount, this->mDescriptorLayout.get());
	// will be deallocated when the pool is destroyed
	this->mDescriptorSetPerFrame = this->mLogicalDevice.mDevice->allocateDescriptorSets(
		vk::DescriptorSetAllocateInfo()
		.setDescriptorPool(this->mDescriptorPool.get())
		.setDescriptorSetCount(frameCount)
		.setPSetLayouts(layouts.data())
	);

	for (uSize i = 0; i < frameCount; ++i)
	{
		std::vector<vk::WriteDescriptorSet> writes;

		auto uniformBufferInfo = vk::DescriptorBufferInfo()
			.setBuffer(*reinterpret_cast<vk::Buffer*>(uniformBufferPerFrame[i].get()))
			.setOffset(0)
			.setRange(uniformBufferRange);
		writes.push_back(
			vk::WriteDescriptorSet()
			.setDstSet(this->mDescriptorSetPerFrame[i])
			.setDstBinding(/*index of the desired binding in bindings*/ 0)
			.setDescriptorType(vk::DescriptorType::eUniformBuffer)
			.setDescriptorCount(uniformDescriptorCount)
			.setDstArrayElement(0)
			.setPBufferInfo(&uniformBufferInfo)
		);

		for (uIndex idxSamplerPair = 0; idxSamplerPair < samplerDescriptorCount; ++idxSamplerPair)
		{
			auto pair = this->mTextureDescriptorPairs[idxSamplerPair];
			auto samplerInfo = vk::DescriptorImageInfo()
				.setImageLayout(vk::ImageLayout::eShaderReadOnlyOptimal);
			samplerInfo
				.setImageView(*reinterpret_cast<vk::ImageView*>(this->mTextureViews[pair.first].get()));
			samplerInfo
				.setSampler(*reinterpret_cast<vk::Sampler*>(this->mTextureSamplers[pair.second].get()));
			writes.push_back(
				vk::WriteDescriptorSet()
				.setDstSet(this->mDescriptorSetPerFrame[i])
				.setDstBinding(/*index of the desired binding in bindings*/ 1)
				.setDescriptorType(vk::DescriptorType::eCombinedImageSampler)
				.setDescriptorCount(samplerDescriptorCount)
				.setDstArrayElement(0)
				.setPImageInfo(&samplerInfo)
			);
		}

		this->mLogicalDevice.mDevice->updateDescriptorSets((ui32)writes.size(), writes.data(), 0, nullptr);
	}
}

void GameRenderer::destroyDescriptorPool()
{
	this->mDescriptorSetPerFrame.clear();
	this->mDescriptorLayout.reset();
	this->mDescriptorPool.reset();
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
				.addAttachment(&this->mFrameImageViews[i])
				.addAttachment(&this->mDepthView)
				.create(&this->mLogicalDevice);
		}
	}

	this->mPipeline.setViewArea(
		vk::Viewport()
		.setX(0).setY(0)
		.setWidth((f32)resolution.width).setHeight((f32)resolution.height)
		.setMinDepth(0.0f).setMaxDepth(1.0f),
		vk::Rect2D().setOffset({ 0, 0 }).setExtent(resolution)
	);
	this->mPipeline.create(&this->mLogicalDevice, &this->mRenderPass, std::vector<vk::DescriptorSetLayout>(1, this->mDescriptorLayout.get()));

	this->mCommandPool
		.setQueueFamily(graphics::QueueFamily::Enum::eGraphics, mPhysicalDevice.queryQueueFamilyGroup())
		.create(&this->mLogicalDevice);
	this->mCommandBuffers = this->mCommandPool.createCommandBuffers(this->mFrameImageViews.size());

	this->recordCommandBufferInstructions();
}

void GameRenderer::recordCommandBufferInstructions()
{
	for (uSize idxFrame = 0; idxFrame < this->mCommandBuffers.size(); ++idxFrame)
	{
		auto cmd = this->mCommandBuffers[idxFrame].beginCommand();
		// Clear commands MUST be passed in the same order as the attachments on framebuffers
		cmd.clearColor({ 0.0f, 0.0f, 0.0f, 1.0f });
		cmd.clearDepth(1.0f, 0);
		cmd.beginRenderPass(&this->mRenderPass, &this->mFrameBuffers[idxFrame]);
		{
			// TODO: Should each render system have control over the descriptor set and pipeline?
			cmd.bindDescriptorSet(&this->mPipeline, &this->mDescriptorSetPerFrame[idxFrame]);
			cmd.bindPipeline(&this->mPipeline);
			for (auto* pRender : this->mpRenders)
			{
				pRender->draw(&cmd);
			}
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
