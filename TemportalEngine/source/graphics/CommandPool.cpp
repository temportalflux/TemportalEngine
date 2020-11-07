#include "graphics/CommandPool.hpp"

#include "graphics/GraphicsDevice.hpp"
#include "graphics/VulkanApi.hpp"

using namespace graphics;

CommandPool::CommandPool()
{
	this->resetConfiguration();
}

CommandPool::CommandPool(CommandPool &&other)
{
	*this = std::move(other);
}

CommandPool& CommandPool::operator=(CommandPool &&other)
{
	this->mInternal.swap(other.mInternal);
	return *this;
}

CommandPool::~CommandPool()
{
	destroy();
}

CommandPool& CommandPool::setFlags(vk::CommandPoolCreateFlags flags)
{
	this->mCreateFlags = flags;
	return *this;
}

CommandPool& CommandPool::setQueueFamily(QueueFamily::Enum queueType, QueueFamilyGroup const &group)
{
	this->mQueueFamily = queueType;
	this->mIdxQueueFamily = group.getQueueIndex(queueType);
	return *this;
}

bool CommandPool::isValid() const
{
	// vulkan unique handles implement bool operator to indicate validity
	return (bool)this->mInternal;
}

void CommandPool::create()
{
	assert(this->mQueueFamily);
	assert(this->mIdxQueueFamily);
	this->mInternal = this->device()->createCommandPool(
		vk::CommandPoolCreateInfo()
		.setFlags(this->mCreateFlags)
		.setQueueFamilyIndex(*this->mIdxQueueFamily)
	);
}

void* CommandPool::get()
{
	return &this->mInternal.get();
}

void CommandPool::invalidate()
{
	this->mInternal.reset();
}

void CommandPool::resetConfiguration()
{
	this->mCreateFlags = vk::CommandPoolCreateFlags();
	this->mQueueFamily = std::nullopt;
	this->mIdxQueueFamily = std::nullopt;
}

std::vector<CommandBuffer> CommandPool::createCommandBuffers(ui32 const count) const
{
	OPTICK_EVENT();
	return this->device()->allocateCommandBuffers(
		this, vk::CommandBufferLevel::ePrimary, count
	);
}

void CommandPool::resetPool()
{
	this->device()->resetPool(this, vk::CommandPoolResetFlags());
}

void CommandPool::submitOneOff(std::function<void(Command* cmd)> writeCommands)
{
	OPTICK_EVENT();
	auto buffers = this->createCommandBuffers(1);
	
	auto cmd = buffers[0].beginCommand(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
	writeCommands(&cmd);
	cmd.end();

	auto queue = this->device()->getQueue(QueueFamily::Enum::eGraphics);
	queue.submit(
		vk::SubmitInfo()
		.setCommandBufferCount(1)
		.setPCommandBuffers(&extract<vk::CommandBuffer>(&buffers[0])),
		vk::Fence()
	);

	queue.waitIdle();
	buffers[0].destroy();
}
