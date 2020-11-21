#include "render/MinecraftRenderer.hpp"

#include "asset/RenderPassAsset.hpp"
#include "graphics/Uniform.hpp"
#include "render/IPipelineRenderer.hpp"

using namespace graphics;

MinecraftRenderer::MinecraftRenderer() : VulkanRenderer()
{
}

void MinecraftRenderer::initializeDevices()
{
	VulkanRenderer::initializeDevices();
	this->initializeTransientCommandPool();

	// TODO: Use a `MemoryChunk` instead of global memory
	this->mpMemoryUniformBuffers = std::make_shared<graphics::Memory>();
	this->mpMemoryUniformBuffers->setDevice(this->getDevice());
	// Host Coherent means this entire buffer will be automatically flushed per write.
	// This can be optimized later by only flushing the portion of the buffer which actually changed.
	this->mpMemoryUniformBuffers->setFlags(vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);

}

void MinecraftRenderer::invalidate()
{
	this->destroyRenderChain();
	this->mpMemoryUniformBuffers.reset();
	this->mCommandPoolTransient.destroy();
	VulkanRenderer::invalidate();
}

void MinecraftRenderer::addMutableUniform(std::string const& key, std::weak_ptr<Uniform> uniform)
{
	this->mpMutableUniforms.insert(std::pair(key, uniform));
}

void MinecraftRenderer::setRenderPass(std::shared_ptr<asset::RenderPass> asset)
{
	// TODO: Don't use default make_shared
	this->mpRenderPass = std::make_shared<graphics::RenderPass>();
	this->mpRenderPass->setDevice(this->getDevice());

	this->mpRenderPass->setClearColor(asset->getClearColor());
	this->mpRenderPass->setClearDepthStencil(asset->getClearDepthStencil());
	this->mpRenderPass->setRenderArea(asset->getRenderArea());

	for (auto phase : asset->getPhases())
	{
		this->mpRenderPass->addPhase(phase);
	}
	for (auto dependency : asset->getPhaseDependencies())
	{
		this->mpRenderPass->addDependency(dependency);
	}
}

void MinecraftRenderer::addRenderer(graphics::IPipelineRenderer *renderer)
{
	renderer->setDevice(this->getDevice());
	renderer->setRenderPass(this->mpRenderPass);
	this->mpRenderers.push_back(renderer);
}

std::shared_ptr<GraphicsDevice> MinecraftRenderer::getDevice()
{
	return this->mpGraphicsDevice;
}

CommandPool& MinecraftRenderer::getTransientPool()
{
	return this->mCommandPoolTransient;
}

void MinecraftRenderer::initializeTransientCommandPool()
{
	auto device = this->getDevice();
	this->mCommandPoolTransient.setDevice(device);
	this->mCommandPoolTransient
		.setFlags(vk::CommandPoolCreateFlagBits::eTransient)
		.setQueueFamily(
			graphics::EQueueFamily::eGraphics,
			device->queryQueueFamilyGroup()
		)
		.create();
}

ui32 MinecraftRenderer::getSwapChainImageViewCount() const
{
	return this->mpGraphicsDevice->querySwapChainSupport().getImageViewCount();
}

void MinecraftRenderer::finalizeInitialization()
{
	VulkanRenderer::finalizeInitialization();
}

void MinecraftRenderer::createRenderChain()
{
	OPTICK_EVENT();
	this->createSwapChain();
	auto resolution = this->mSwapChain.getResolution();
	
	this->createFrameImageViews();
	this->createDepthResources(resolution);
	this->createRenderPass();

	auto viewCount = this->mFrameImageViews.size();
	
	for (auto& renderer : this->mpRenderers)
	{
		renderer->setFrameCount(viewCount);
		renderer->createDescriptors(this->getDevice());
		renderer->createPipeline(resolution);
	}

	this->createFrames(viewCount);
	this->createMutableUniformBuffers();

	for (auto& renderer : this->mpRenderers)
	{
		renderer->attachDescriptors(this->mMutableUniformBuffersByDescriptorId);
		renderer->writeDescriptors(this->getDevice());
	}
}

void MinecraftRenderer::destroyRenderChain()
{
	OPTICK_EVENT();
	this->destroyFrames();
	for (auto& renderer : this->mpRenderers)
	{
		renderer->destroyRenderChain();
	}
	this->destroyRenderPass();
	this->mMutableUniformBuffersByDescriptorId.clear();
	this->destroyDepthResources();
	this->destroyFrameImageViews();
	this->destroySwapChain();
}

void MinecraftRenderer::createMutableUniformBuffers()
{
	if (this->mpMutableUniforms.size() > 0)
	{
		auto device = this->getDevice();

		auto viewCount = this->getSwapChainImageViewCount();

		for (auto const& uniformEntry : this->mpMutableUniforms)
		{
			auto const uniformBindingId = uniformEntry.first;
			auto buffers = std::vector<graphics::Buffer*>();
			for (uIndex i = 0; i < viewCount; ++i)
			{
				auto& frame = this->mFrames[i];
				graphics::Buffer buffer;
				buffer.setDevice(this->mpGraphicsDevice);
				buffer
					.setUsage(vk::BufferUsageFlagBits::eUniformBuffer)
					.setSize(uniformEntry.second.lock()->size())
					.create();
				buffer.configureSlot(this->mpMemoryUniformBuffers);
				frame.uniformBuffers.insert(std::pair<std::string, graphics::Buffer>(uniformBindingId, std::move(buffer)));
				buffers.push_back(&frame.uniformBuffers[uniformBindingId]);
			}
			this->mMutableUniformBuffersByDescriptorId.insert(std::pair(uniformBindingId, buffers));
		}
		this->mpMemoryUniformBuffers->create();
		for (uIndex i = 0; i < viewCount; ++i)
		{
			for (auto& entry : this->mFrames[i].uniformBuffers)
			{
				entry.second.bindMemory();
			}
		}
	}
}

void MinecraftRenderer::createDepthResources(math::Vector2UInt const &resolution)
{
	OPTICK_EVENT();
	auto device = this->getDevice();
	auto& transientCmdPool = this->getTransientPool();

	auto supportedFormat = device->pickFirstSupportedFormat(
		{ vk::Format::eD32Sfloat, vk::Format::eD32SfloatS8Uint, vk::Format::eD24UnormS8Uint },
		vk::ImageTiling::eOptimal, vk::FormatFeatureFlagBits::eDepthStencilAttachment
	);
	if (!supportedFormat) throw std::runtime_error("failed to find supported depth buffer format");

	this->mpMemoryDepthImage = std::make_shared<Memory>();
	this->mpMemoryDepthImage->setDevice(device);
	this->mpMemoryDepthImage->setFlags(vk::MemoryPropertyFlagBits::eDeviceLocal);

	this->mDepthImage.setDevice(device);
	this->mDepthImage
		.setFormat(*supportedFormat)
		.setSize({ resolution.x(), resolution.y(), 1 })
		.setTiling(vk::ImageTiling::eOptimal)
		.setUsage(vk::ImageUsageFlagBits::eDepthStencilAttachment);
	this->mDepthImage.create();

	this->mDepthImage.configureSlot(this->mpMemoryDepthImage);
	this->mpMemoryDepthImage->create();
	this->mDepthImage.bindMemory();
	// TODO this should not wait for idle GPU, and instead use the pipeline barrier
	this->mDepthImage.transitionLayout(vk::ImageLayout::eUndefined, vk::ImageLayout::eDepthStencilAttachmentOptimal, &transientCmdPool);

	this->mDepthView.setDevice(this->mpGraphicsDevice);
	this->mDepthView
		.setImage(&this->mDepthImage, vk::ImageAspectFlagBits::eDepth)
		.create();
}

void MinecraftRenderer::destroyDepthResources()
{
	this->mDepthView.invalidate();
	this->mDepthImage.invalidate();
	this->mpMemoryDepthImage.reset();
}

void MinecraftRenderer::createRenderPass()
{
	OPTICK_EVENT();
	getRenderPass()
		->setImageFormatType(graphics::ImageFormatReferenceType::Enum::Viewport, (ui32)this->mSwapChain.getFormat())
		.setImageFormatType(graphics::ImageFormatReferenceType::Enum::Depth, (ui32)this->mDepthImage.getFormat())
		.create();
}

RenderPass* MinecraftRenderer::getRenderPass()
{
	return this->mpRenderPass.get();
}

void MinecraftRenderer::destroyRenderPass()
{
	this->mpRenderPass->invalidate();
}

void MinecraftRenderer::createFrames(uSize viewCount)
{	
	OPTICK_EVENT();
	auto device = this->getDevice();
	auto resolution = this->mSwapChain.getResolution();
	auto queueFamilyGroup = device->queryQueueFamilyGroup();

	this->mFrames.resize(viewCount);
	for (uIndex i = 0; i < viewCount; ++i)
	{
		auto& frame = this->mFrames[i];

		frame.commandPool.setDevice(device);
		frame.commandPool
			.setQueueFamily(graphics::EQueueFamily::eGraphics, queueFamilyGroup)
			.create();
		auto commandBuffers = frame.commandPool.createCommandBuffers(1);
		frame.commandBuffer = std::move(commandBuffers[0]);

		frame.frameBuffer
			.setRenderPass(this->getRenderPass())
			.setResolution(this->mSwapChain.getResolution())
			.addAttachment(&this->mFrameImageViews[i])
			.addAttachment(&this->mDepthView)
			.create(device);

		frame.frame.create(device);
	}
}

uSize MinecraftRenderer::getNumberOfFrames() const
{
	return this->mFrames.size();
}

graphics::Frame* MinecraftRenderer::getFrameAt(uSize idx)
{
	return &this->mFrames[idx].frame;
}

void MinecraftRenderer::destroyFrames()
{
	for (auto& frame : this->mFrames)
	{
		frame.commandBuffer.destroy();
		frame.commandPool.destroy();
		frame.frameBuffer.destroy();
		frame.frame.destroy();
	}
	this->mFrames.clear();
}

void MinecraftRenderer::record(uIndex idxFrame)
{
	OPTICK_EVENT();
	auto& frame = this->mFrames[idxFrame];
	frame.commandPool.resetPool();
	auto cmd = frame.commandBuffer.beginCommand();
	cmd.beginRenderPass(this->getRenderPass(), &frame.frameBuffer, this->mSwapChain.getResolution());
	for (auto* pRender : this->mpRenderers)
	{
		pRender->record(&cmd, idxFrame);
	}
	cmd.endRenderPass();
	cmd.end();
}

void MinecraftRenderer::prepareRender(ui32 idxCurrentFrame)
{
	OPTICK_EVENT();
	this->record(idxCurrentFrame);
	VulkanRenderer::prepareRender(idxCurrentFrame);
}

void MinecraftRenderer::render(graphics::Frame* currentFrame, ui32 idxCurrentImage)
{
	OPTICK_EVENT();
	// Submit the command buffer to the graphics queue
	auto& commandBuffer = this->mFrames[idxCurrentImage].commandBuffer;
	currentFrame->submitBuffers(&this->getQueue(EQueueFamily::eGraphics), { &commandBuffer });
}

void MinecraftRenderer::onFramePresented(uIndex idxFrame)
{
	OPTICK_EVENT();
	auto& buffers = this->mFrames[idxFrame].uniformBuffers;
	for (auto& element : this->mpMutableUniforms)
	{
		std::shared_ptr<Uniform> uniform = element.second.lock();
		if (uniform->hasChanged())
		{
			uniform->beginReading();
			buffers[element.first].write(
				/*offset*/ 0, uniform->data(), uniform->size()
			);
			uniform->endReading(true);
		}
	}
	this->UpdateWorldGraphicsOnFramePresented.execute();
}
