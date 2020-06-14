#include "graphics/GameRenderer.hpp"

#include "graphics/Uniform.hpp"

using namespace graphics;

GameRenderer::GameRenderer()
	: VulkanRenderer()
{
	this->mVertexBuffer
		.setUsage(vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eVertexBuffer)
		.setMemoryRequirements(vk::MemoryPropertyFlagBits::eDeviceLocal);
	this->mIndexBuffer
		.setUsage(vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eIndexBuffer)
		.setMemoryRequirements(vk::MemoryPropertyFlagBits::eDeviceLocal);
}

void GameRenderer::invalidate()
{
	this->destroyRenderChain();
	this->mPipeline.clearShaders();

	this->destroyInputBuffers();

	VulkanRenderer::invalidate();
}

void GameRenderer::setStaticUniform(std::shared_ptr<Uniform> uniform)
{
	this->mpUniformStatic = uniform;
}

void GameRenderer::createInputBuffers(ui64 vertexBufferSize, ui64 indexBufferSize)
{
	this->mVertexBuffer.setSize(vertexBufferSize).create(&this->mLogicalDevice);
	this->mIndexBuffer.setSize(indexBufferSize).create(&this->mLogicalDevice);

	this->mCommandPoolTransient
		.setQueueFamily(
			graphics::QueueFamily::Enum::eGraphics,
			this->mPhysicalDevice.queryQueueFamilyGroup()
		)
		.create(&this->mLogicalDevice, vk::CommandPoolCreateFlagBits::eTransient);
}

void GameRenderer::writeToBuffer(Buffer* buffer, ui64 offset, void* data, ui64 size)
{
	Buffer& stagingBuffer = Buffer()
		.setUsage(vk::BufferUsageFlagBits::eTransferSrc)
		.setMemoryRequirements(
			vk::MemoryPropertyFlagBits::eHostVisible
			| vk::MemoryPropertyFlagBits::eHostCoherent
		)
		.setSize(size);
	stagingBuffer.create(&this->mLogicalDevice);
	stagingBuffer.write(&this->mLogicalDevice, offset, data, size);
	this->copyBetweenBuffers(&stagingBuffer, buffer, size);
	stagingBuffer.destroy();
}

void GameRenderer::copyBetweenBuffers(Buffer *src, Buffer *dest, ui64 size)
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


void GameRenderer::destroyInputBuffers()
{
	this->mVertexBuffer.destroy();
	this->mIndexBuffer.destroy();
}

void GameRenderer::setBindings(std::vector<AttributeBinding> bindings)
{
	this->mPipeline.setBindings(bindings);
}

void GameRenderer::addShader(std::shared_ptr<ShaderModule> shader)
{
	this->mPipeline.addShader(shader);
}

void GameRenderer::createRenderChain()
{
	this->createRenderObjects();
	this->createUniformBuffers();
	this->createDescriptorPool();
	this->createCommandObjects();
	this->createFrames(this->mImageViews.size());
}

void GameRenderer::destroyRenderChain()
{
	this->destroyFrames();
	this->destroyCommandObjects();
	this->destroyDescriptorPool();
	this->destroyUniformBuffers();
	this->destroyRenderObjects();
}

void GameRenderer::createUniformBuffers()
{
	this->mUniformStaticBuffersPerFrame.resize(this->mImageViews.size());
	for (auto& buffer : this->mUniformStaticBuffersPerFrame)
	{
		buffer
			.setUsage(vk::BufferUsageFlagBits::eUniformBuffer)
			// Host Coherent means this entire buffer will be automatially flushed per write.
			// This can be optimized later by only flushing the portion of the buffer which actually changed.
			.setMemoryRequirements(vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent)
			.setSize(this->mpUniformStatic->size())
			.create(&this->mLogicalDevice);
	}
}

void GameRenderer::destroyUniformBuffers()
{
	this->mUniformStaticBuffersPerFrame.clear();
}

void GameRenderer::createDescriptorPool()
{
	auto setCount = (ui32)this->mImageViews.size();
	auto uniformDescriptorCount = 1;
	auto idxUniformBufferBinding = 0;

	std::vector<vk::DescriptorPoolSize> poolSizes = {
		// Uniform Buffer Pool size
		vk::DescriptorPoolSize()
			.setType(vk::DescriptorType::eUniformBuffer)
			.setDescriptorCount(setCount * uniformDescriptorCount)
	};
	this->mDescriptorPool_StaticUniform = this->mLogicalDevice.mDevice->createDescriptorPoolUnique(
		vk::DescriptorPoolCreateInfo()
		.setPoolSizeCount((ui32)poolSizes.size()).setPPoolSizes(poolSizes.data())
		.setMaxSets(setCount)
	);

	std::vector<vk::DescriptorSetLayoutBinding> bindings = {
		// Uniform Buffer Binding
		vk::DescriptorSetLayoutBinding()
		.setBinding(idxUniformBufferBinding)
		.setDescriptorType(vk::DescriptorType::eUniformBuffer)
		.setDescriptorCount(uniformDescriptorCount)
		.setStageFlags(vk::ShaderStageFlagBits::eVertex)
	};
	this->mDescriptorLayout_StaticUniform = this->mLogicalDevice.mDevice->createDescriptorSetLayoutUnique(
		vk::DescriptorSetLayoutCreateInfo()
		.setBindingCount((ui32)bindings.size()).setPBindings(bindings.data())
	);

	std::vector<vk::DescriptorSetLayout> layouts(setCount, this->mDescriptorLayout_StaticUniform.get());
	// will be deallocated when the pool is destroyed
	this->mDescriptorSetPerFrame_StaticUniform = this->mLogicalDevice.mDevice->allocateDescriptorSets(
		vk::DescriptorSetAllocateInfo()
		.setDescriptorPool(this->mDescriptorPool_StaticUniform.get())
		.setDescriptorSetCount(setCount)
		.setPSetLayouts(layouts.data())
	);

	for (uSize i = 0; i < setCount; ++i)
	{
		auto uniformBufferInfo = vk::DescriptorBufferInfo()
			.setBuffer(*reinterpret_cast<vk::Buffer*>(this->mUniformStaticBuffersPerFrame[i].get()))
			.setOffset(0)
			.setRange(this->mpUniformStatic->size());

		auto writeDescriptorUniform = vk::WriteDescriptorSet()
			.setDescriptorType(vk::DescriptorType::eUniformBuffer)
			.setDescriptorCount(uniformDescriptorCount)
			.setDstSet(this->mDescriptorSetPerFrame_StaticUniform[i])
			.setDstBinding(idxUniformBufferBinding).setDstArrayElement(0)
			.setPBufferInfo(&uniformBufferInfo);

		std::vector<vk::WriteDescriptorSet> writes = { writeDescriptorUniform };
		this->mLogicalDevice.mDevice->updateDescriptorSets((ui32)writes.size(), writes.data(), 0, nullptr);
	}
}

void GameRenderer::destroyDescriptorPool()
{
	this->mDescriptorSetPerFrame_StaticUniform.clear();
	this->mDescriptorLayout_StaticUniform.reset();
	this->mDescriptorPool_StaticUniform.reset();
}

void GameRenderer::createCommandObjects()
{
	auto resolution = this->mSwapChain.getResolution();

	this->mFrameBuffers = this->mRenderPass.createFrameBuffers(this->mImageViews);

	this->mPipeline.setViewArea(
		vk::Viewport()
		.setX(0).setY(0)
		.setWidth((f32)resolution.width).setHeight((f32)resolution.height)
		.setMinDepth(0.0f).setMaxDepth(1.0f),
		vk::Rect2D().setOffset({ 0, 0 }).setExtent(resolution)
	);
	{
		std::vector<vk::VertexInputBindingDescription> bindings = {
			vk::VertexInputBindingDescription()
			.setBinding(/*index of binding in bindings*/ 0)
			.setInputRate(/*per vertex or per instance*/ vk::VertexInputRate::eVertex)
			.setStride(/*size*/ 0)
		};
		std::vector<vk::VertexInputAttributeDescription> attributes = {
			vk::VertexInputAttributeDescription()
			.setBinding(/*index of binding in bindings that this attrib is connected to*/ 0)
			.setLocation(/*layout(location = slot) ...*/ 0)
			.setFormat(/*data type of the attribute*/ vk::Format::eR32G32B32Sfloat /*= float vec3*/)
		};
		auto infoInputVertex = vk::PipelineVertexInputStateCreateInfo()
			.setVertexBindingDescriptionCount((ui32)bindings.size())
			.setPVertexBindingDescriptions(bindings.data())
			.setVertexAttributeDescriptionCount((ui32)attributes.size())
			.setPVertexAttributeDescriptions(attributes.data());
	}
	this->mPipeline.create(&this->mLogicalDevice, &this->mRenderPass, std::vector<vk::DescriptorSetLayout>(1, this->mDescriptorLayout_StaticUniform.get()));

	this->mCommandPool
		.setQueueFamily(graphics::QueueFamily::Enum::eGraphics, mPhysicalDevice.queryQueueFamilyGroup())
		.create(&this->mLogicalDevice);
	this->mCommandBuffers = this->mCommandPool.createCommandBuffers(this->mImageViews.size());

	this->recordCommandBufferInstructions();
}

void GameRenderer::recordCommandBufferInstructions()
{
	for (uSize idxFrame = 0; idxFrame < this->mCommandBuffers.size(); ++idxFrame)
	{
		auto cmd = this->mCommandBuffers[idxFrame].beginCommand();
		cmd.clear({ 0.0f, 0.0f, 0.0f, 1.0f });
		cmd.beginRenderPass(&this->mRenderPass, &this->mFrameBuffers[idxFrame]);
		{
			// TODO: This should perform a draw call for each object type being rendered
			// TODO: This should handle instancing of each object type and send model or model-view matrix via instanced uniform data
			cmd
				.bindDescriptorSet(&this->mPipeline, &this->mDescriptorSetPerFrame_StaticUniform[idxFrame])
				.bindPipeline(&this->mPipeline)
				.bindVertexBuffers({ &this->mVertexBuffer })
				.bindIndexBuffer(0, &this->mIndexBuffer, this->mIndexBufferUnitType)
				.draw(this->mIndexCount);
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
