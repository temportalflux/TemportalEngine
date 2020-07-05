#include "graphics/CommandPool.hpp"

#include "graphics/GraphicsDevice.hpp"

using namespace graphics;

CommandPool& CommandPool::setQueueFamily(QueueFamily::Enum queueType, QueueFamilyGroup const &group)
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

CommandPool& CommandPool::create(std::shared_ptr<GraphicsDevice> device, vk::CommandPoolCreateFlags flags)
{
	this->mpDevice = device;
	this->mInternal = device->createCommandPool(
		vk::CommandPoolCreateInfo()
		.setFlags(flags)
		.setQueueFamilyIndex(this->mIdxQueueFamily)
	);
	return *this;
}

void CommandPool::destroy()
{
	this->mpDevice.reset();
	this->mInternal.reset();
}

std::vector<CommandBuffer> CommandPool::createCommandBuffers(ui32 const count) const
{
	return this->mpDevice.lock()->allocateCommandBuffers(
		this, vk::CommandBufferLevel::ePrimary, count
	);
}

void CommandPool::resetPool()
{
	this->mpDevice.lock()->resetPool(this, vk::CommandPoolResetFlags());
}
