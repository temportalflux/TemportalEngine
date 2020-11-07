#include "graphics/CommandBuffer.hpp"

using namespace graphics;

CommandBuffer::CommandBuffer(CommandBuffer &&other)
{
	*this = std::move(other);
}

CommandBuffer& CommandBuffer::operator=(CommandBuffer &&other)
{
	this->mInternal.swap(other.mInternal);
	return *this;
}

CommandBuffer::~CommandBuffer()
{
	destroy();
}

void* CommandBuffer::get()
{
	return &this->mInternal.get();
}

void CommandBuffer::invalidate()
{
	this->mInternal.reset();
}

Command CommandBuffer::beginCommand(vk::CommandBufferUsageFlags flags)
{
	OPTICK_EVENT();
	this->mInternal->begin(vk::CommandBufferBeginInfo().setFlags(flags));
	return Command(this);
}

void CommandBuffer::endCommand(Command *command)
{
	this->mInternal->end();
}
