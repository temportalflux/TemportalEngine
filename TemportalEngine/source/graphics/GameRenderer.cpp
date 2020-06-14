#include "graphics/GameRenderer.hpp"

using namespace graphics;

GameRenderer::GameRenderer()
	: VulkanRenderer()
{

}

void GameRenderer::createRenderChain()
{
	VulkanRenderer::createRenderChain();

}

void GameRenderer::destroyRenderChain()
{
	VulkanRenderer::destroyRenderChain();
}

void GameRenderer::createUniformBuffers()
{
	this->mUniformBuffers.resize(this->mImageViews.size());
	for (auto& buffer : this->mUniformBuffers)
	{
		buffer
			.setUsage(vk::BufferUsageFlagBits::eUniformBuffer)
			.setMemoryRequirements(vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent)
			.setSize(this->mTotalUniformSize)
			.create(&this->mLogicalDevice);
	}
}

void GameRenderer::destroyUniformBuffers()
{
	this->mUniformBuffers.clear();
}

void GameRenderer::createDescriptorPool()
{
	auto setCount = (ui32)this->mImageViews.size();
	auto uniformDescriptorCount = (ui32)this->mUniformPts.size();
	auto idxUniformBufferBinding = 0;

	std::vector<vk::DescriptorPoolSize> poolSizes = {
		// Uniform Buffer Pool size
		vk::DescriptorPoolSize()
			.setType(vk::DescriptorType::eUniformBuffer)
			.setDescriptorCount(setCount * uniformDescriptorCount)
	};
	this->mDescriptorPool = this->mLogicalDevice.mDevice->createDescriptorPoolUnique(
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
	this->mDescriptorLayout = this->mLogicalDevice.mDevice->createDescriptorSetLayoutUnique(
		vk::DescriptorSetLayoutCreateInfo()
		.setBindingCount((ui32)bindings.size()).setPBindings(bindings.data())
	);

	std::vector<vk::DescriptorSetLayout> layouts(setCount, this->mDescriptorLayout.get());
	// will be deallocated when the pool is destroyed
	this->mDescriptorSets = this->mLogicalDevice.mDevice->allocateDescriptorSets(
		vk::DescriptorSetAllocateInfo()
		.setDescriptorPool(this->mDescriptorPool.get())
		.setDescriptorSetCount(setCount)
		.setPSetLayouts(layouts.data())
	);

	for (uSize i = 0; i < setCount; ++i)
	{
		auto uniformBufferInfo = vk::DescriptorBufferInfo()
			.setBuffer(*reinterpret_cast<vk::Buffer*>(this->mUniformBuffers[i].get()))
			.setOffset(0)
			.setRange(this->mTotalUniformSize);

		auto writeDescriptorUniform = vk::WriteDescriptorSet()
			.setDescriptorType(vk::DescriptorType::eUniformBuffer)
			.setDescriptorCount(uniformDescriptorCount)
			.setDstSet(this->mDescriptorSets[i])
			.setDstBinding(idxUniformBufferBinding).setDstArrayElement(0)
			.setPBufferInfo(&uniformBufferInfo);

		std::vector<vk::WriteDescriptorSet> writes = { writeDescriptorUniform };
		this->mLogicalDevice.mDevice->updateDescriptorSets((ui32)writes.size(), writes.data(), 0, nullptr);
	}
}

void GameRenderer::destroyDescriptorPool()
{
	this->mDescriptorSets.clear();
	this->mDescriptorLayout.reset();
	this->mDescriptorPool.reset();
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
	this->mPipeline.create(&this->mLogicalDevice, &this->mRenderPass, std::vector<vk::DescriptorSetLayout>(1, this->mDescriptorLayout.get()));

	this->mCommandPool
		.setQueueFamily(graphics::QueueFamily::Enum::eGraphics, mPhysicalDevice.queryQueueFamilyGroup())
		.create(&this->mLogicalDevice);
	this->mCommandBuffers = this->mCommandPool.createCommandBuffers(this->mImageViews.size());

	this->recordCommandBufferInstructions();
}

void GameRenderer::recordCommandBufferInstructions()
{
	for (uSize i = 0; i < this->mCommandBuffers.size(); ++i)
	{
		this->mCommandBuffers[i].beginCommand()
			.clear({ 0.0f, 0.0f, 0.0f, 1.0f })
			.beginRenderPass(&this->mRenderPass, &this->mFrameBuffers[i])
			.bindPipeline(&this->mPipeline)
			.bindDescriptorSet(&this->mPipeline, &this->mDescriptorSets[i])
			.bindVertexBuffers({ &this->mVertexBuffer })
			.bindIndexBuffer(0, &this->mIndexBuffer, this->mIndexBufferUnitType)
			.draw(this->mIndexCount)
			.endRenderPass()
			.end();
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
