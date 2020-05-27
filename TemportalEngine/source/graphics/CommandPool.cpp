#include "graphics/CommandPool.hpp"

#include "graphics/LogicalDevice.hpp"

using namespace graphics;

CommandPool& CommandPool::setQueueFamily(QueueFamily queueType, QueueFamilyGroup const &group)
{
	auto optQueueIdx = group.getQueueIndex(queueType);
	assert(optQueueIdx.has_value());

	this->mQueueFamily = queueType;
	this->mIdxQueueFamily = optQueueIdx.value();

	return *this;
}

bool CommandPool::isValid() const
{
	// vulkan unique handles implement bool operator to indicate validity
	return (bool)this->mInternal;
}

CommandPool& CommandPool::create(LogicalDevice const *pDevice, vk::CommandPoolCreateFlags flags)
{
	this->mInternal = (this->mpDevice = pDevice)->mDevice->createCommandPoolUnique(
		vk::CommandPoolCreateInfo()
		.setFlags(flags)
		.setQueueFamilyIndex(this->mIdxQueueFamily)
	);
	return *this;
}

void CommandPool::destroy()
{
	this->mInternal.reset();
}

std::vector<CommandBuffer> CommandPool::createCommandBuffers(uSize count) const
{
	auto bufferHandles = mpDevice->mDevice->allocateCommandBuffersUnique(vk::CommandBufferAllocateInfo()
		.setLevel(vk::CommandBufferLevel::ePrimary)
		.setCommandPool(this->mInternal.get())
		.setCommandBufferCount((ui32)count)
	);
	auto buffers = std::vector<CommandBuffer>(count);
	for (uSize i = 0; i < count; ++i)
	{
		buffers[i].mInternal.swap(bufferHandles[i]);
	}
	return buffers;
}

void CommandPool::resetPool()
{
	this->mpDevice->mDevice->resetCommandPool(this->mInternal.get(), vk::CommandPoolResetFlags());
}
