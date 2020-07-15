#include "graphics/CommandPool.hpp"

#include "graphics/GraphicsDevice.hpp"
#include "graphics/VulkanApi.hpp"

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

void CommandPool::submitOneOff(std::function<void(Command* cmd)> writeCommands)
{
	auto buffers = this->createCommandBuffers(1);
	
	auto cmd = buffers[0].beginCommand(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
	writeCommands(&cmd);
	cmd.end();

	auto queue = this->mpDevice.lock()->getQueue(QueueFamily::Enum::eGraphics);
	queue.submit(
		vk::SubmitInfo()
		.setCommandBufferCount(1)
		.setPCommandBuffers(&extract<vk::CommandBuffer>(&buffers[0])),
		vk::Fence()
	);

	queue.waitIdle();
}
