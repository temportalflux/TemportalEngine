#include "render/ImmediateRenderSystem.hpp"

#include "asset/RenderPassAsset.hpp"
#include "graphics/Uniform.hpp"
#include "render/IPipelineRenderer.hpp"

using namespace graphics;

ImmediateRenderSystem::ImmediateRenderSystem() : VulkanRenderer()
{
}

void ImmediateRenderSystem::initializeDevices()
{
	VulkanRenderer::initializeDevices();
	this->initializeTransientCommandPool();

	this->mGlobalDescriptorPool.setDevice(this->getDevice());
	this->mGlobalDescriptorPool.setPoolSize(64, {
		{ vk::DescriptorType::eUniformBuffer, 64 },
		{ vk::DescriptorType::eCombinedImageSampler, 64 },
	});
	this->mGlobalDescriptorPool.setAllocationMultiplier(this->getSwapChainImageViewCount());
	this->mGlobalDescriptorPool.create();

	this->mFrames.resize(this->getSwapChainImageViewCount());
}

void ImmediateRenderSystem::invalidate()
{
	this->destroyRenderChain();
	this->mFrames.clear();
	this->mGlobalMutableDescriptors.clear();
	this->mMutableUniformBuffersByDescriptorId.clear();
	this->mGlobalDescriptorPool.destroy();
	this->mCommandPoolTransient.destroy();
	VulkanRenderer::invalidate();
}

DescriptorPool& ImmediateRenderSystem::getDescriptorPool()
{
	return this->mGlobalDescriptorPool;
}

void ImmediateRenderSystem::addGlobalMutableDescriptor(std::string const& layoutId, uSize const& bindingCount)
{
	auto gmd = GlobalMutableDescriptor {};
	gmd.layout.setDevice(this->getDevice());
	gmd.layout.setBindingCount(bindingCount);
	this->mGlobalMutableDescriptors.insert(std::make_pair(layoutId, std::move(gmd)));
}

void ImmediateRenderSystem::addMutableUniform(std::string const& key, std::weak_ptr<Uniform> uniform)
{
	this->mpMutableUniforms.insert(std::pair(key, uniform));
}

void ImmediateRenderSystem::addMutableUniformToLayout(
	std::string const& layoutId, std::string const& uniformId,
	uIndex const& bindingIndex,
	graphics::DescriptorType const& type, graphics::ShaderStage const& stage
)
{
	auto& gmd = this->mGlobalMutableDescriptors.find(layoutId)->second;
	gmd.mutableUniformIds.push_back(uniformId);
	gmd.layout.setBinding(bindingIndex, uniformId, type, stage, 1);
}

void ImmediateRenderSystem::setRenderPass(std::shared_ptr<asset::RenderPass> asset)
{
	// TODO: Don't use default make_shared
	this->mpRenderPass = std::make_shared<graphics::RenderPass>();
	this->mpRenderPass->setDevice(this->getDevice());

	this->mpRenderPass->setClearColor(asset->getClearColor());
	this->mpRenderPass->setClearDepthStencil(asset->getClearDepthStencil());
	this->mpRenderPass->setRenderArea(asset->getRenderArea());

	auto attachments = std::vector<graphics::RenderPassAttachment>();
	for (auto const& attachment : asset->getAttachments()) attachments.push_back(attachment.data);
	this->mpRenderPass->setAttachments(attachments);

	auto phases = std::vector<graphics::RenderPassPhase>();
	for (auto const& phase : asset->getPhases()) phases.push_back(phase.data);
	this->mpRenderPass->setPhases(phases);

	auto dependencies = std::vector<graphics::RenderPassDependency>();
	for (auto const& dependency : asset->getPhaseDependencies()) dependencies.push_back(dependency.data);
	this->mpRenderPass->setPhaseDependencies(dependencies);
}

void ImmediateRenderSystem::addRenderer(graphics::IPipelineRenderer *renderer)
{
	renderer->setDevice(this->getDevice());
	renderer->setRenderPass(this->mpRenderPass);
	renderer->initializeData(&this->getTransientPool(), &this->getDescriptorPool());
	this->mpRenderers.push_back(renderer);
}

CommandPool& ImmediateRenderSystem::getTransientPool()
{
	return this->mCommandPoolTransient;
}

void ImmediateRenderSystem::initializeTransientCommandPool()
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

ui32 ImmediateRenderSystem::getSwapChainImageViewCount() const
{
	return this->mpGraphicsDevice->querySwapChainSupport().getImageViewCount();
}

void ImmediateRenderSystem::createMutableUniforms()
{
	this->createMutableUniformBuffers();
	for (auto&[layoutId, gmd] : this->mGlobalMutableDescriptors)
	{
		gmd.layout.create();
		gmd.sets.resize(this->getSwapChainImageViewCount());
		gmd.layout.createSets(&this->getDescriptorPool(), gmd.sets);
		for (uIndex idxSet = 0; idxSet < gmd.sets.size(); ++idxSet)
		{
			auto& set = gmd.sets[idxSet];
			for (auto const& uniformId : gmd.mutableUniformIds)
			{
				set.attach(uniformId, this->mMutableUniformBuffersByDescriptorId[uniformId][idxSet]);
			}
			set.writeAttachments();
		}
	}
}

void ImmediateRenderSystem::finalizeInitialization()
{
	VulkanRenderer::finalizeInitialization();
}

std::unordered_map<std::string, graphics::DescriptorLayout const*> ImmediateRenderSystem::getGlobalDescriptorLayouts() const
{
	auto layouts = std::unordered_map<std::string, graphics::DescriptorLayout const*>();
	for (auto& entry : this->mGlobalMutableDescriptors)
	{
		layouts.insert(std::make_pair(entry.first, &entry.second.layout));
	}
	return layouts;
}

graphics::DescriptorSet const* ImmediateRenderSystem::getGlobalDescriptorSet(std::string const& layoutId, uIndex idxFrame) const
{
	return &(this->mGlobalMutableDescriptors.find(layoutId)->second.sets[idxFrame]);
}

void ImmediateRenderSystem::setDPI(ui32 dotsPerInch)
{
	this->mDPI = dotsPerInch;
	this->markRenderChainDirty();
}

ui32 ImmediateRenderSystem::dpi() const { return this->mDPI; }

void ImmediateRenderSystem::createRenderChain()
{
	OPTICK_EVENT();
	this->createSwapChain();
	auto resolution = this->getResolution();
	
	this->createFrameImageViews();
	this->createDepthResources(resolution);
	this->createRenderPass();

	auto viewCount = this->mFrameImageViews.size();

	for (auto& renderer : this->mpRenderers)
	{
		renderer->setFrameCount(viewCount);
		renderer->createDescriptors(&this->getDescriptorPool());
		renderer->setDescriptorLayouts(this->getGlobalDescriptorLayouts());
		renderer->createPipeline(resolution);
	}

	this->createFrames(viewCount);

	for (auto& renderer : this->mpRenderers)
	{
		renderer->attachDescriptors(this->mMutableUniformBuffersByDescriptorId);
		renderer->writeDescriptors(this->getDevice());
	}
}

void ImmediateRenderSystem::destroyRenderChain()
{
	OPTICK_EVENT();
	this->destroyFrames();
	for (auto& renderer : this->mpRenderers)
	{
		renderer->destroyRenderChain();
	}
	this->destroyRenderPass();
	this->destroyDepthResources();
	this->destroyFrameImageViews();
	this->destroySwapChain();
}

void ImmediateRenderSystem::createMutableUniformBuffers()
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
					.setUsage(vk::BufferUsageFlagBits::eUniformBuffer, MemoryUsage::eCPUToGPU)
					.setSize(uniformEntry.second.lock()->size())
					.create();
				frame.uniformBuffers.insert(std::pair<std::string, graphics::Buffer>(uniformBindingId, std::move(buffer)));
				buffers.push_back(&frame.uniformBuffers[uniformBindingId]);
			}
			this->mMutableUniformBuffersByDescriptorId.insert(std::pair(uniformBindingId, buffers));
		}
	}
}

void ImmediateRenderSystem::createDepthResources(math::Vector2UInt const &resolution)
{
	OPTICK_EVENT();
	auto device = this->getDevice();
	auto& transientCmdPool = this->getTransientPool();

	auto supportedFormat = device->pickFirstSupportedFormat(
		{ vk::Format::eD32Sfloat, vk::Format::eD32SfloatS8Uint, vk::Format::eD24UnormS8Uint },
		vk::ImageTiling::eOptimal, vk::FormatFeatureFlagBits::eDepthStencilAttachment
	);
	if (!supportedFormat) throw std::runtime_error("failed to find supported depth buffer format");

	this->mDepthImage.setDevice(device);
	this->mDepthImage
		.setFormat(*supportedFormat)
		.setSize({ resolution.x(), resolution.y(), 1 })
		.setTiling(vk::ImageTiling::eOptimal)
		.setUsage(vk::ImageUsageFlagBits::eDepthStencilAttachment);
	this->mDepthImage.create();

	// TODO this should not wait for idle GPU, and instead use the pipeline barrier
	this->mDepthImage.transitionLayout(vk::ImageLayout::eUndefined, vk::ImageLayout::eDepthStencilAttachmentOptimal, &transientCmdPool);

	this->mDepthView.setDevice(this->mpGraphicsDevice);
	this->mDepthView
		.setImage(&this->mDepthImage, vk::ImageAspectFlagBits::eDepth)
		.create();
}

void ImmediateRenderSystem::destroyDepthResources()
{
	this->mDepthView.invalidate();
	this->mDepthImage.invalidate();
}

void ImmediateRenderSystem::createRenderPass()
{
	OPTICK_EVENT();
	getRenderPass()
		->setImageFormatType(graphics::EImageFormatCategory::Viewport, (ui32)this->mSwapChain.getFormat())
		.setImageFormatType(graphics::EImageFormatCategory::Depth, (ui32)this->mDepthImage.getFormat())
		.create();
}

RenderPass* ImmediateRenderSystem::getRenderPass()
{
	return this->mpRenderPass.get();
}

void ImmediateRenderSystem::destroyRenderPass()
{
	this->mpRenderPass->invalidate();
}

void ImmediateRenderSystem::createFrames(uSize viewCount)
{	
	OPTICK_EVENT();
	auto device = this->getDevice();
	auto resolution = this->getResolution();
	auto queueFamilyGroup = device->queryQueueFamilyGroup();

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
			.setResolution(resolution)
			.addAttachment(&this->mFrameImageViews[i])
			.addAttachment(&this->mDepthView)
			.create(device);

		frame.frame.create(device);
	}
}

uSize ImmediateRenderSystem::getNumberOfFrames() const
{
	return this->mFrames.size();
}

graphics::Frame* ImmediateRenderSystem::getFrameAt(uSize idx)
{
	return &this->mFrames[idx].frame;
}

void ImmediateRenderSystem::destroyFrames()
{
	for (auto& frame : this->mFrames)
	{
		frame.commandBuffer.destroy();
		frame.commandPool.destroy();
		frame.frameBuffer.destroy();
		frame.frame.destroy();
	}
}

void ImmediateRenderSystem::record(uIndex idxFrame)
{
	OPTICK_EVENT();
	IPipelineRenderer::TGetGlobalDescriptorSet getDescrSet = std::bind(
		&ImmediateRenderSystem::getGlobalDescriptorSet, this,
		std::placeholders::_1, std::placeholders::_2
	);

	auto& frame = this->mFrames[idxFrame];
	frame.commandPool.resetPool();
	auto cmd = frame.commandBuffer.beginCommand();
	cmd.beginRenderPass(this->getRenderPass(), &frame.frameBuffer, this->getResolution());
	for (auto* pRender : this->mpRenderers)
	{
		pRender->record(&cmd, idxFrame, getDescrSet);
	}
	cmd.endRenderPass();
	cmd.end();
}

void ImmediateRenderSystem::prepareRender(ui32 idxCurrentFrame)
{
	OPTICK_EVENT();
	this->record(idxCurrentFrame);
	VulkanRenderer::prepareRender(idxCurrentFrame);
}

void ImmediateRenderSystem::render(graphics::Frame* currentFrame, ui32 idxCurrentImage)
{
	OPTICK_EVENT();
	// Submit the command buffer to the graphics queue
	auto& commandBuffer = this->mFrames[idxCurrentImage].commandBuffer;
	currentFrame->submitBuffers(&this->getQueue(EQueueFamily::eGraphics), { &commandBuffer });
}

void ImmediateRenderSystem::onFramePresented(uIndex idxFrame)
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
